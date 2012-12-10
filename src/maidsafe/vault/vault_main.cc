/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include <signal.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <mutex>
#include <thread>

#include "boost/filesystem.hpp"
#include "boost/program_options.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/tokenizer.hpp"

#include "maidsafe/common/config.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/private/lifestuff_manager/vault_controller.h"

#include "maidsafe/pd/client/utils.h"
#include "maidsafe/pd/common/key_manager.h"
#include "maidsafe/pd/common/return_codes.h"
#include "maidsafe/pd/vault/node.h"
#include "maidsafe/pd/vault/utils.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

const std::string kVaultVersion = "MaidSafe PD Vault " + maidsafe::kApplicationVersion;

std::mutex mutex_;
std::condition_variable cond_var_;
bool ctrlc_pressed(false);

void ctrlc_handler(int signum) {
  LOG(kInfo) << " Signal received: " << signum;
  boost::mutex::scoped_lock lock(mutex_);
  ctrlc_pressed = true;
  cond_var_.notify_one();
}

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << kVaultVersion << std::endl;

#ifdef NDEBUG
  try {
#endif
    boost::system::error_code error_code;
    std::string config_file;
    int identity_index;
    // Options allowed only on command line
    po::options_description generic_options("General options");
    generic_options.add_options()
        ("help,h", "Print this help message")
//         ("version,v", "Display version")
        ("start", "Start vault")
        ("stop", "Stop vault")
//         ("first_node,f", po::bool_switch(), "First node of the network.")
        ("config", po::value(&config_file)->default_value(config_file), "Path to config file");

    // Options allowed both on command line and in config file
    po::options_description config_file_options("Configuration options");
    config_file_options.add_options()
        ("chunk_path", po::value<std::string>(), "Directory to store chunks in")
        ("cache_capacity", po::value<uintmax_t>(), "Memory allocated to caching")
#ifndef __APPLE__
        ("plugins_path", po::value<std::string>(), "Path to statistics plugins (enables stats)")
#endif
        ("vmid", po::value<std::string>(), "ID to identify to vault manager")
        ("peer", po::value<std::string>(), "Endpoint of bootstrap node")
        ("keys_path",
            po::value<std::string>()->default_value(
                fs::path(fs::temp_directory_path(error_code) / "key_directory.dat").string()),
            "Path to keys file for bootstrapping")
        ("identity_index", po::value<int>(&identity_index)->default_value(-1),
            "Entry from keys file to use as ID")
        ("usr_id",
            po::value<std::string>()->default_value("lifestuff"),
            "user id if running under in non-win OS and from inside a process");

    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_file_options);

    po::variables_map variables_map;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).allow_unregistered().
                                                  allow_unregistered().run(), variables_map);
    po::notify(variables_map);

    if (variables_map.count("help") ||
        !(variables_map.count("start") || variables_map.count("stop"))) {
      std::cout << cmdline_options << std::endl;
      return 0;
    }

    if (variables_map.count("stop"))
      return 0;

    // loading config file
    if (!config_file.empty()) {
      fs::path config_path(config_file);
      if (fs::exists(config_path, error_code)) {
        std::ifstream config_file_stream(config_path.string().c_str());
        po::store(po::parse_config_file(config_file_stream, config_file_options), variables_map);
        po::notify(variables_map);
      } else {
        LOG(kError) << "No configuration file at " << config_path.string() << " ("
                    << error_code.message() << ")";
      }
    }

    fs::path chunk_path = maidsafe::pd::vault::GetPathFromProgramOption(
        "chunk_path", &variables_map, true, true);
    if (chunk_path.empty()) {
      std::cout << "Can't start vault without a chunk storage location set." << std::endl;
      return 1;
    }

    std::string usr_id("lifestuff");
    if (variables_map.count("usr_id")) {
      usr_id = variables_map.at("usr_id").as<std::string>();
    }
    maidsafe::priv::lifestuff_manager::VaultController vault_controller(usr_id);
    bool using_vault_controller(false);
    if (variables_map.count("vmid")) {
      std::string vmid = variables_map.at("vmid").as<std::string>();
      if (!vault_controller.Start(vmid, []() { ctrlc_handler(SIGTERM); })) {
        LOG(kError) << "Could not start vault controller.";
        using_vault_controller = false;
      } else {
        using_vault_controller = true;
      }
    }

    // bootstrap endpoint
    std::vector<boost::asio::ip::udp::endpoint> peer_endpoints;
    if (variables_map.count("peer")) {
      std::string peer = variables_map.at("peer").as<std::string>();
      size_t delim = peer.rfind(':');
      try {
        boost::asio::ip::udp::endpoint ep;
        ep.port(boost::lexical_cast<uint16_t>(peer.substr(delim + 1)));
        ep.address(boost::asio::ip::address::from_string(peer.substr(0, delim)));
        peer_endpoints.push_back(ep);
        LOG(kInfo) << "Going to bootstrap off endpoint " << ep;
      }
      catch(...) {
        std::cout << "Could not parse IPv4 peer endpoint from " << peer << std::endl;
      }
    }

    auto node = std::make_shared<maidsafe::pd::vault::Node>();

    // Cache capacity
    if (variables_map.count("cache_capacity")) {
      uintmax_t cache_capacity(variables_map.at("cache_capacity").as<uintmax_t>());
      node->set_cache_capacity(cache_capacity);
    }

    maidsafe::Fob fob;
    maidsafe::pd::AccountName account_name;

    boost::filesystem::path keys_path(maidsafe::pd::vault::GetPathFromProgramOption(
        "keys_path", &variables_map, false, false));
    if (fs::exists(keys_path, error_code)) {
      auto all_keys = maidsafe::pd::ReadKeyDirectory(keys_path);
      for (size_t i = 0; i < all_keys.size(); ++i) {
        if (!node->key_manager()->AddKey(all_keys[i].first.identity,
                                         all_keys[i].first.keys.public_key,
                                         all_keys[i].first.validation_token) ||
            !node->key_manager()->AddKey(all_keys[i].second.identity,
                                         all_keys[i].second.keys.public_key,
                                         all_keys[i].second.validation_token)) {
          std::cout << "Could not add identity #" << i << " from keys file." << std::endl;
          return 1;
        }
        if (static_cast<int>(i) == identity_index) {
          fob = all_keys[i].second;
          account_name = maidsafe::pd::AccountName(all_keys[i].first.identity);
          LOG(kInfo) << "Using identity #" << i << " from keys file.";
        }
      }
      LOG(kInfo) << "Added " << all_keys.size() << " keys.";
    }
    std::vector<std::pair<std::string, uint16_t>> endpoints_from_lifestuff_manager;
    // Set up identity
    if (!fob.identity.IsInitialised()) {
      std::string name;
      if (!using_vault_controller ||
          (using_vault_controller &&
           !vault_controller.GetIdentity(fob, name, endpoints_from_lifestuff_manager))) {
        std::cout << "No identity available, can't start vault." << std::endl;
        return 1;
      }
      account_name = maidsafe::pd::AccountName(name);
    }

    for (auto it(endpoints_from_lifestuff_manager.rbegin());
         it != endpoints_from_lifestuff_manager.rend();
        ++it) {
      boost::asio::ip::udp::endpoint ep;
      ep.port((*it).second);
      ep.address(boost::asio::ip::address::from_string((*it).first));
      peer_endpoints.push_back(ep);
      LOG(kVerbose) << "Received bootstrap endpoint " << ep << " from LifeStuffManager";
    }

    node->set_fob(fob);
    node->set_account_name(account_name.IsInitialised() ? account_name
                                                        : maidsafe::pd::AccountName(fob.identity));
    if (using_vault_controller)
      node->set_on_new_bootstrap_endpoint(
          [&vault_controller](const boost::asio::ip::udp::endpoint &endpoint) {
            std::pair<std::string, uint16_t> endpoint_pair;
            endpoint_pair.first = endpoint.address().to_string();
            endpoint_pair.second = endpoint.port();
            vault_controller.SendEndpointToLifeStuffManager(endpoint_pair);
          });

    // Vault statistics
#ifndef __APPLE__
    if (variables_map.count("plugins_path")) {
      boost::filesystem::path plugins_path(variables_map.at("plugins_path").as<std::string>());
      if (!plugins_path.empty())
        node->set_vault_stats_needed(true, plugins_path);
    }
#endif

    // Starting Vault
    std::cout << "Starting vault..." << std::endl;
    int result = node->Start(chunk_path, peer_endpoints);
    if (using_vault_controller)
      vault_controller.ConfirmJoin(result == maidsafe::pd::kSuccess);
    if (result != maidsafe::pd::kSuccess) {
      std::cout << "Error during vault start-up. (" << result << ")" << std::endl;
      return 1;
    }


#ifndef WIN32
    signal(SIGHUP, ctrlc_handler);
#endif

    signal(SIGINT, ctrlc_handler);
    signal(SIGTERM, ctrlc_handler);
    std::cout << "Vault running as " << maidsafe::HexSubstr(node->fob().identity) << std::endl;
    {
      boost::mutex::scoped_lock lock(mutex_);
      cond_var_.wait(lock, [] { return ctrlc_pressed; });  // NOLINT
    }

    std::cout << "Stopping vault..." << std::endl;
    result = node->Stop();
    if (result != maidsafe::pd::kSuccess) {
      std::cout << "Error during vault shutdown. (" << result << ")" << std::endl;
      return 1;
    }
#ifdef NDEBUG
  }
  catch(const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return 1;
  }
  catch(...) {
    std::cout << "Exception of unknown type." << std::endl;
    return 1;
  }
#endif

  return 0;
}
