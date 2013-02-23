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

#include "maidsafe/vault/tools/commander.h"

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << maidsafe::vault::tools::kHelperVersion << std::endl;

  int result(0);

  size_t pmids_count(12), chunk_set_count(3);

  try {
    maidsafe::vault::tools::Commander commander(pmids_count, chunk_set_count);
    commander.AnalyseCommandLineOptions(argc, argv);

//     if (do_extended) {
//       if (all_pmids.size() < 5) {
//         std::cout << "Need to have at least 5 keys to run extended test." << std::endl;
//         result = -1;
//       } else if (!ExtendedTest(chunk_set_count, all_pmids, peer_endpoints)) {
//         std::cout << "Extended test failed." << std::endl;
//         result = -1;
//       }
//     }
  }
  catch(const std::exception& exception) {
    std::cout << "Error: " << exception.what() << std::endl;
    result = -1;
  }
  catch(...) {
    std::cout << "Unknown exception." << std::endl;
    result = -2;
  }

  return result;
}
