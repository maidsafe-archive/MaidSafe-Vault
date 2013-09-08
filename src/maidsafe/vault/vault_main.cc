/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include <signal.h>

#include <condition_variable>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

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
#include "maidsafe/vault/tools/tools_exception.h"


namespace maidsafe {

namespace vault {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

std::mutex g_mutex;
std::condition_variable g_cond_var;
std::atomic<bool> g_ctrlc_pressed(false);

void SigHandler(int signum) {
  LOG(kInfo) << " Signal received: " << signum;
  g_ctrlc_pressed.store(true);
  g_cond_var.notify_one();
}

fs::path GetPathFromProgramOption(const std::string& option_name,
                                  po::variables_map& variables_map,
                                  bool is_dir,
                                  bool create_new_if_absent) {
  fs::path option_path(variables_map.at(option_name).as<std::string>());
  if (!fs::exists(option_path)) {
    if (!create_new_if_absent)
      throw tools::ToolsException("Invalid " + option_name + ", " + option_path.string() +
                                  " doesn't exist or can't be accessed");
  } else if (is_dir && !fs::is_directory(option_path)) {
    throw tools::ToolsException(option_name + " path " + option_path.string() +
                                " isn't a directory.");
  } else if (!is_dir && !fs::is_regular_file(option_path)) {
    throw tools::ToolsException(option_name + " path " + option_path.string() +
                                " isn't a regular file.");
  }

  if (is_dir && create_new_if_absent) {  // Create new dir
    fs::create_directories(option_path);
  } else if (!is_dir && create_new_if_absent) {  // Create new file
    std::ofstream ofs(option_path.c_str());
    ofs.close();
  }

  LOG(kInfo) << "GetPathFromProgramOption - " << option_name << " is " << option_path;
  return option_path;
}

boost::asio::ip::udp::endpoint GetEndpointFromString(const std::string& string_ep) {
  size_t delim(string_ep.rfind(':'));
  boost::asio::ip::udp::endpoint ep;
  ep.port(static_cast<uint16_t>(std::stoi(string_ep.substr(delim + 1).c_str())));
  ep.address(boost::asio::ip::address::from_string(string_ep.substr(0, delim)));
  return ep;
}

#ifdef TESTING
void GetIdentityAndEndpoints(po::variables_map& variables_map,
                             std::unique_ptr<passport::Pmid>& pmid,
                             std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints,
                             std::vector<passport::PublicPmid>& pmids) {
  if (variables_map.count("peer") == 0) {
    routing::Parameters::append_local_live_port_endpoint = true;
  } else {
    peer_endpoints.push_back(GetEndpointFromString(variables_map.at("peer").as<std::string>()));
  }

  if (variables_map.count("identity_index") == 0) {
    std::cout << "No identity selected" << std::endl;
    throw tools::ToolsException("No identity selected");
  }

  fs::path keys_path(GetPathFromProgramOption("keys_path", variables_map, false, false));
  std::vector<passport::detail::AnmaidToPmid> key_chains(
      passport::detail::ReadKeyChainList(keys_path));
  size_t identity_index(variables_map.at("identity_index").as<int>());
  if (identity_index >= key_chains.size()) {
    std::cout << "Identity selected out of bounds" << std::endl;
    throw tools::ToolsException("Identity selected out of bounds");
  }
  pmid.reset(new passport::Pmid(key_chains.at(identity_index).pmid));
  for (auto& key_chain : key_chains)
    pmids.push_back(passport::PublicPmid(key_chain.pmid));
}
#endif

void RunVault(po::variables_map& variables_map) {
  fs::path chunk_path(GetPathFromProgramOption("chunk_path", variables_map, true, true));
  std::vector<boost::asio::ip::udp::endpoint> peer_endpoints;
  std::unique_ptr<passport::Pmid> pmid;
  std::vector<passport::PublicPmid> pmids;
#ifdef TESTING
  GetIdentityAndEndpoints(variables_map, pmid, peer_endpoints, pmids);
#endif
  std::string vmid(variables_map.count("vmid") == 0 ?
                       "test" : variables_map.at("vmid").as<std::string>());
  lifestuff_manager::VaultController vault_controller(vmid, [] {
                                                              g_ctrlc_pressed.store(true);
                                                              g_cond_var.notify_one();
                                                            });
  if (vmid != "test" && !vault_controller.GetIdentity(pmid, peer_endpoints)) {
    std::cout << "Failed to get ID from VC" << std::endl;
    ThrowError(CommonErrors::uninitialised);
  }

#ifndef MAIDSAFE_WIN32
  signal(SIGHUP, SigHandler);
#endif
  signal(SIGTERM, SigHandler);
  bool disable_ctrl_c(variables_map.at("disable_ctrl_c").as<bool>());
  if (!disable_ctrl_c)
    signal(SIGINT, SigHandler);

  // Starting Vault
  std::cout << "Starting vault..." << std::endl;
  Vault vault(*pmid,
              chunk_path,
              [] (const boost::asio::ip::udp::endpoint&) {},
              pmids,
              peer_endpoints);
  std::cout << "Vault running as " << maidsafe::HexSubstr(pmid->name().data) << std::endl;
  {
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cond_var.wait(lock, [] { return g_ctrlc_pressed.load(); });  // NOLINT
  }

  std::cout << "Stopping vault..." << std::endl;
}

#ifdef TESTING
void AddTestingOptions(po::options_description& config_file_options) {
  config_file_options.add_options()
      ("peer", po::value<std::string>(), "Endpoint of bootstrap node")
      ("keys_path",
       po::value<std::string>()->default_value(fs::path(fs::temp_directory_path() /
                                                        "key_directory.dat").string()),
       "Path to keys file for bootstrapping")
      ("identity_index", po::value<int>(), "Entry from keys file to use as ID")
      ("disable_ctrl_c", po::value<bool>()->default_value(false), "disable ctrl+c");
}
#endif

bool InvalidOptions(po::variables_map& variables_map) {
#ifdef TESTING
  if (variables_map.count("identity_index") == 0 ||
      variables_map.at("identity_index").as<int>() < 2)
    return true;
#else
  if (variables_map.count("vmid") == 0)
    return true;
#endif
  if (variables_map.count("chunk_path") == 0)
    return true;

  return false;
}

po::options_description PopulateVariablesMap(int argc,
                                             char* argv[],
                                             po::variables_map& variables_map) {
  po::options_description generic_options("General options");
  generic_options.add_options()
      ("help,h", "Print this help message")
      ("version,v", "Display version");

  po::options_description config_file_options("Configuration options");
  config_file_options.add_options()
#ifndef __APPLE__
      ("plugins_path", po::value<std::string>(), "Path to statistics plugins (enables stats)")
#endif
      ("chunk_path", po::value<std::string>(), "Directory to store chunks in")
      ("vmid", po::value<std::string>(), "ID to identify to vault manager");
#ifdef TESTING
  AddTestingOptions(config_file_options);
#endif

  po::options_description cmdline_options;
  cmdline_options.add(generic_options).add(config_file_options);

  po::store(po::command_line_parser(argc, argv).options(cmdline_options).allow_unregistered().run(),
            variables_map);
  po::notify(variables_map);

  return cmdline_options;
}

void ActOnOptions(int argc, char* argv[]) {
  po::variables_map variables_map;
  po::options_description cmdline_options(PopulateVariablesMap(argc, argv, variables_map));

  if (variables_map.count("version") != 0) {
    std::cout << "Lifestuff Vault " + kApplicationVersion() << std::endl;
    return;
  }

  bool invalid_options(InvalidOptions(variables_map));
  if (variables_map.count("help") != 0 || invalid_options) {
    std::cout << cmdline_options << std::endl;
    if (invalid_options)
      throw tools::ToolsException("Invalid options");
  }

  RunVault(variables_map);
}

}  // namespace vault

}  // namespace maidsafe

// this should
// 1: Start a vault_controller
// 2: start vault object
// All additional code should be refactored to the 40 line limit and
// placed behind ifdef TESTING
int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);
  try {
    maidsafe::vault::ActOnOptions(argc, argv);
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
