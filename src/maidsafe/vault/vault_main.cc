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

#include <condition_variable>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <mutex>

#include "boost/filesystem/convenience.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/program_options.hpp"

#include "maidsafe/common/config.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/return_codes.h"

#include "maidsafe/lifestuff_manager/vault_controller.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/vault.h"


namespace fs = boost::filesystem;
namespace po = boost::program_options;

namespace {

std::mutex g_mutex;
std::condition_variable g_cond_var;
bool g_ctrlc_pressed(false);

void SigHandler(int signum) {
  LOG(kInfo) << " Signal received: " << signum;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_ctrlc_pressed = true;
  }
  g_cond_var.notify_one();
}

fs::path GetPathFromProgramOption(const std::string& option_name,
                                  po::variables_map* variables_map,
                                  bool is_dir,
                                  bool create_new_if_absent) {
  fs::path option_path;
  if (variables_map->count(option_name))
    option_path = variables_map->at(option_name).as<std::string>();
  if (option_path.empty())
    return fs::path();

  boost::system::error_code ec;
  if (!fs::exists(option_path, ec) || ec) {
    if (!create_new_if_absent) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " doesn't exist or can't be accessed (" << ec.message() << ")";
      return fs::path();
    }

    if (is_dir) {  // Create new dir
      fs::create_directories(option_path, ec);
      if (ec) {
        LOG(kError) << "GetPathFromProgramOption - Unable to create new dir " << option_path << " ("
                    << ec.message() << ")";
        return fs::path();
      }
    } else {  // Create new file
      if (option_path.has_filename()) {
        try {
          std::ofstream ofs(option_path.c_str());
        }
        catch(const std::exception &e) {
          LOG(kError) << "GetPathFromProgramOption - Exception while creating new file: "
                      << e.what();
          return fs::path();
        }
      }
    }
  }

  if (is_dir) {
    if (!fs::is_directory(option_path, ec) || ec) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " is not a directory (" << ec.message() << ")";
      return fs::path();
    }
  } else {
    if (!fs::is_regular_file(option_path, ec) || ec) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " is not a regular file (" << ec.message() << ")";
      return fs::path();
    }
  }

  LOG(kInfo) << "GetPathFromProgramOption - " << option_name << " is " << option_path;
  return option_path;
}

int ProcessOption(po::variables_map& variables_map, int identity_index) {
  fs::path chunk_path = GetPathFromProgramOption("chunk_path", &variables_map, true, true);
  if (chunk_path.empty()) {
    std::cout << "Can't start vault without a chunk storage location set." << std::endl;
    return 1;
  }

  std::string usr_id("lifestuff");
  if (variables_map.count("usr_id"))
    usr_id = variables_map.at("usr_id").as<std::string>();

  //  maidsafe::lifestuff_manager::VaultController vault_controller(usr_id);
  //  bool using_vault_controller(false);
//  std::string vmid(usr_id);
//  if (variables_map.count("vmid"))
//    vmid = variables_map.at("vmid").as<std::string>();

//  if (!vault_controller.Start(vmid, []() { SigHandler(SIGTERM); })) {
//    LOG(kError) << "Could not start vault controller.";
//    using_vault_controller = false;
//  } else {
//    using_vault_controller = true;
//  }

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

  boost::system::error_code error_code;
  // Load keys
  fs::path keys_path(GetPathFromProgramOption("keys_path", &variables_map, false, false));
  std::unique_ptr<maidsafe::passport::Pmid> pmid;
  std::vector<maidsafe::passport::PublicPmid> all_public_pmids;
  if (fs::exists(keys_path, error_code)) {
    auto all_pmids = maidsafe::passport::detail::ReadPmidList(keys_path);
    pmid = std::unique_ptr<maidsafe::passport::Pmid>(
               new maidsafe::passport::Pmid(all_pmids[identity_index]));
    LOG(kInfo) << "Added " << all_pmids.size() << " keys. Using identity #" << identity_index
               << " from keys file.";
    all_public_pmids.reserve(all_pmids.size());
    for (auto& pmid : all_pmids)
      all_public_pmids.push_back(maidsafe::passport::PublicPmid(pmid));
  }
//  std::cout << "all_public_pmids: " << all_public_pmids.size() << std::endl;

//  std::vector<std::pair<std::string, uint16_t>> endpoints_from_lifestuff_manager;
//  // Set up identity via vault_controller, if not from file
//  if (!(*pmid).name().data.IsInitialised()) {
//    if (!using_vault_controller ||
//        (using_vault_controller &&
//            !vault_controller.GetIdentity(pmid, endpoints_from_lifestuff_manager))) {
//      std::cout << "No identity available, can't start vault." << std::endl;
//      return 3;
//    }
//  }

#ifndef MAIDSAFE_WIN32
  signal(SIGHUP, SigHandler);
#endif
  signal(SIGINT, SigHandler);
  signal(SIGTERM, SigHandler);

  // Starting Vault
  std::cout << "Starting vault..." << std::endl;

  // To allow bootstrapping off vaults on local machine
  maidsafe::routing::Parameters::append_local_live_port_endpoint = true;

  try {
    auto vault = std::make_shared<maidsafe::vault::Vault>(
      *pmid,
      chunk_path,
      [/*&vault_controller*/] (const boost::asio::ip::udp::endpoint &/*endpoint*/) {
//        std::pair<std::string, uint16_t> endpoint_pair;
//        endpoint_pair.first = endpoint.address().to_string();
//        endpoint_pair.second = endpoint.port();
//        vault_controller.SendEndpointToLifeStuffManager(endpoint_pair);
      },
      all_public_pmids,
      peer_endpoints);

//  int result(maidsafe::routing::ReturnCode::kSuccess);
//  if (using_vault_controller)
//    vault_controller.ConfirmJoin(result == maidsafe::routing::ReturnCode::kSuccess);
//  if (result != maidsafe::routing::ReturnCode::kSuccess) {
//    std::cout << "Error during vault start-up. (" << result << ")" << std::endl;
//    return 4;
//  }

    std::cout << "Vault running as "
              << maidsafe::HexSubstr((*pmid).name().data.string()) << std::endl;
    {
      std::unique_lock<std::mutex> lock(g_mutex);
      g_cond_var.wait(lock, [] { return g_ctrlc_pressed; });  // NOLINT
    }

    std::cout << "Stopping vault..." << std::endl;
  }
  catch(const maidsafe::maidsafe_error& e) {
    LOG(kError) << "Maidsafe exception: " << e.what();
    return -1;
  }
  catch(const std::exception& e) {
    LOG(kError) << "Standard exception: " << e.what();
    return -2;
  }
  catch(...) {
    LOG(kError) << "Unknown exception";
    return -3;
  }

  return 0;
}

int OptionMenu(int argc, char* argv[]) {
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
//        ("version,v", "Display version")
//        ("start", "Start vault")
//        ("stop", "Stop vault")
//        ("first_node,f", po::bool_switch(), "First node of the network.")
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
                                                  allow_unregistered().run(),
              variables_map);
    po::notify(variables_map);

    if (variables_map.count("help")) {
      std::cout << cmdline_options << std::endl;
      return 0;
    }

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

    return ProcessOption(variables_map, identity_index);
#ifdef NDEBUG
  }
  catch(const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return 6;
  }
  catch(...) {
    std::cout << "Exception of unknown type." << std::endl;
    return 7;
  }
#endif
}

}  // unnamed namespace

// this should
// 1: Start a vault_controller
// 2: start vault object
// All additional code should be refactored to the 40 line limit and
// placed behind ifdef TESTING
int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << "MaidSafe PD Vault " + maidsafe::kApplicationVersion + "\n";

  OptionMenu(argc, argv);

  return 0;
}
