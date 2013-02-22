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

#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/program_options/parsers.hpp"

#include "maidsafe/common/config.h"

#include "maidsafe/vault/tests/key_helper_operations.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << maidsafe::vault::tools::kHelperVersion << std::endl;

  int result(0);
  boost::system::error_code error_code;

  size_t pmids_count(12), chunk_set_count(3);

#ifdef NDEBUG
  try {
#endif
    // Options allowed only on command line
    po::options_description generic_options("Commands");
    generic_options.add_options()
        ("help,h", "Print this help message.")
        ("create,c", "Create keys and write to file.")
        ("load,l", "Load keys from file.")
        ("run,r", "Run vaults with available keys.")
        ("bootstrap,b", "Run boostrap nodes only, using first 2 keys.")
        ("store,s", "Store keys on network.")
        ("verify,v", "Verify keys are available on network.")
        ("test,t", "Run simple test that stores and retrieves chunks.")
        ("extended,x", "Run extended test that tries all operations on all chunk types.")
        ("delete,d", "Delete keys file.")
        ("print,p", "Print the list of keys available.");

    // Options allowed both on command line and in config file
    po::options_description config_file_options("Configuration options");
    config_file_options.add_options()
        ("peer",
         po::value<std::string>(),
         "Endpoint of bootstrap node, if attaching to running network.")
        ("pmids_count,n",
         po::value<size_t>(&pmids_count)->default_value(pmids_count),
         "Number of keys to create")
        ("keys_path",
         po::value<std::string>()->default_value(
             boost::filesystem::path(boost::filesystem::temp_directory_path(error_code) /
                                     "key_directory.dat").string()),
         "Path to keys file")
        ("chunk_path",
         po::value<std::string>()->default_value(
             boost::filesystem::path(boost::filesystem::temp_directory_path(error_code) /
                                     "keys_chunks").string()),
         "Path to chunk directory")
        ("chunk_set_count",
         po::value<size_t>(&chunk_set_count)->default_value(chunk_set_count),
         "Number of chunk sets to run extended test on");

    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_file_options);

    po::variables_map variables_map;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).allow_unregistered().
                                                  allow_unregistered().run(), variables_map);
    po::notify(variables_map);

    bool do_create(variables_map.count("create") != 0);
    bool do_load(variables_map.count("load") != 0);
    bool do_run(variables_map.count("run") != 0);
    bool do_bootstrap(variables_map.count("bootstrap") != 0);
    bool do_store(variables_map.count("store") != 0);
    bool do_verify(variables_map.count("verify") != 0);
    bool do_test(variables_map.count("test") != 0);
    bool do_extended(variables_map.count("extended") != 0);
    bool do_delete(variables_map.count("delete") != 0);
    bool do_print(variables_map.count("print") != 0);

    if (variables_map.count("help") ||
        (!do_create && !do_load && !do_run && !do_bootstrap && !do_store && !do_verify &&
         !do_test && !do_extended && !do_delete && !do_print)) {
      std::cout << cmdline_options << std::endl
                << "Commands are executed in this order: [c|l] p [r|b] s v t x d" << std::endl;
      return 0;
    }

    maidsafe::vault::tools::PmidVector all_pmids;
    fs::path keys_path(maidsafe::vault::tools::GetPathFromProgramOption("keys_path",
                                                                        &variables_map,
                                                                        false,
                                                                        true));
//    fs::path chunk_path(GetPathFromProgramOption("chunk_path", &variables_map, true, true));

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

    if (do_create) {
      if (maidsafe::vault::tools::CreateKeys(pmids_count, all_pmids)) {
        std::cout << "Created " << all_pmids.size() << " pmids." << std::endl;
        if (maidsafe::passport::detail::WritePmidList(keys_path, all_pmids))
          std::cout << "Wrote keys to " << keys_path << std::endl;
        else
          std::cout << "Could not write keys to " << keys_path << std::endl;
      } else {
        std::cout << "Could not create keys." << std::endl;
      }
    } else if (do_load) {
      try {
        all_pmids = maidsafe::passport::detail::ReadPmidList(keys_path);
        std::cout << "Loaded " << all_pmids.size() << " pmids from " << keys_path << std::endl;
      }
      catch(const std::exception& /*ex*/) {
        all_pmids.clear();
        std::cout << "Could not load keys from " << keys_path << std::endl;
      }
    }

    if (do_print)
      maidsafe::vault::tools::PrintKeys(all_pmids);

    if (do_run || do_bootstrap) {
      if (all_pmids.size() < 3) {
        std::cout << "Need to have at least 3 keys to setup network." << std::endl;
        result = -1;
      } else if (!maidsafe::vault::tools::SetupNetwork(all_pmids, !do_run && do_bootstrap)) {
        std::cout << "Could not setup network." << std::endl;
        result = -1;
      } else if (do_run) {
        std::cout << "Vaults are running." << std::endl;
      }
    }

    if (do_store) {
      if (maidsafe::vault::tools::StoreKeys(all_pmids, peer_endpoints)) {
        std::cout << "Stored keys on network." << std::endl;
      } else {
        std::cout << "Could not store all keys on network." << std::endl;
        result = -1;
      }
    }

    if (do_verify) {
      if (maidsafe::vault::tools::VerifyKeys(all_pmids, peer_endpoints)) {
        std::cout << "Verified keys on network." << std::endl;
      } else {
        std::cout << "Could not verify all keys on network." << std::endl;
        result = -1;
      }
    }

    if (do_test) {
      if (all_pmids.size() < 4) {
        std::cout << "Need to have at least 4 keys to run test." << std::endl;
        result = -1;
      } else if (!maidsafe::vault::tools::StoreChunks(all_pmids, peer_endpoints)) {
        std::cout << "Could not store and retrieve test chunks." << std::endl;
        result = -1;
      }
    }

//     if (do_extended) {
//       if (all_pmids.size() < 5) {
//         std::cout << "Need to have at least 5 keys to run extended test." << std::endl;
//         result = -1;
//       } else if (!ExtendedTest(chunk_set_count, all_pmids, peer_endpoints)) {
//         std::cout << "Extended test failed." << std::endl;
//         result = -1;
//       }
//     }

    if (do_delete) {
      if (fs::remove(keys_path, error_code))
        std::cout << "Deleted " << keys_path << std::endl;
      else
        std::cout << "Could not delete " << keys_path << std::endl;
    }
#ifdef NDEBUG
  }
  catch(const std::exception& exception) {
    std::cout << "Error: " << exception.what() << std::endl;
    result = -2;
  }
#endif

  return result;
}
