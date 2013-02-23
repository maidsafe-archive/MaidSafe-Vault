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

#include "maidsafe/vault/tools/key_helper_operations.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

namespace maidsafe {

namespace vault {

namespace tools {

class Commander {
 public:
  Commander(size_t pmids_count, size_t chunk_set_count);
  void AnalyseCommandLineOptions(int argc, char* argv[]);

 private:
  size_t pmids_count_, chunk_set_count_;
  maidsafe::vault::tools::PmidVector all_pmids_;
  fs::path keys_path_;
  std::vector<boost::asio::ip::udp::endpoint> peer_endpoints_;

  struct SelectedOperationsContainer {
    SelectedOperationsContainer()
        : do_create(false),
          do_load(false),
          do_run(false),
          do_bootstrap(false),
          do_store(false),
          do_verify(false),
          do_test(false),
          do_extended(false),
          do_delete(false),
          do_print(false) {}
    bool do_create, do_load, do_run, do_bootstrap, do_store, do_verify, do_test, do_extended,
         do_delete, do_print;
  } selected_ops_;

  boost::asio::ip::udp::endpoint GetBootstrapEndpoint(const std::string& peer);
  po::options_description AddGenericOptions(const std::string& title);
  po::options_description AddConfigurationOptions(const std::string& title);
  po::variables_map CheckOptionValidity(po::options_description& cmdline_options,
                                        int argc,
                                        char* argv[]);
  void HandleKeys();
  void HandleNetWork();
  bool HandleStore();
  bool HandleVerify();
  bool HandleDoTest();
  void HandleDeleteKeys();
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe
