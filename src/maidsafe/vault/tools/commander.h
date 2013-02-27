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

namespace maidsafe {

namespace vault {

namespace tools {

class SelectedOperationsContainer {
 public:
  SelectedOperationsContainer()
      : do_create(false),
        do_load(false),
        do_bootstrap(false),
        do_store(false),
        do_verify(false),
        do_test(false),
        do_delete(false),
        do_print(false) {}
  bool do_create, do_load, do_bootstrap, do_store, do_verify, do_test, do_delete, do_print;
  bool InvalidOptions(const boost::program_options::variables_map& variables_map,
                      const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);

 private:
  bool NoOptionsSelected() const;
  bool ConflictedOptions(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) const;
};

class Commander {
 public:
  Commander(size_t pmids_count);
  void AnalyseCommandLineOptions(int argc, char* argv[]);

 private:
  size_t pmids_count_;
  maidsafe::vault::tools::PmidVector all_pmids_;
  boost::filesystem::path keys_path_;
  std::vector<boost::asio::ip::udp::endpoint> peer_endpoints_;
  SelectedOperationsContainer selected_ops_;

  boost::asio::ip::udp::endpoint GetBootstrapEndpoint(const std::string& peer);
  boost::program_options::options_description AddGenericOptions(const std::string& title);
  boost::program_options::options_description AddConfigurationOptions(const std::string& title);
  void CheckOptionValidity(boost::program_options::options_description& cmdline_options,
                           int argc,
                           char* argv[]);

  void ChooseOperations();
  void HandleKeys();
  void HandleSetupBootstraps();
  void HandleStore();
  void HandleVerify();
  void HandleDoTest();
  void HandleDeleteKeys();

  void CreateKeys();
  void GetPathFromProgramOption(const std::string& option_name,
                                boost::program_options::variables_map& variables_map);
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe
