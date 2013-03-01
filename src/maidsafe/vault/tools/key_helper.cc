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

#include "maidsafe/common/error.h"

#include "maidsafe/vault/tools/commander.h"
#include "maidsafe/vault/tools/tools_exception.h"

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << maidsafe::vault::tools::kHelperVersion << std::endl;

  try {
    maidsafe::vault::tools::Commander commander(12);
    commander.AnalyseCommandLineOptions(argc, argv);
  }
  catch(const maidsafe::vault::tools::ToolsException& exception) {
    std::cout << "Tools Exception: " << exception.what() << std::endl;
    return -1;
  }
  catch(const maidsafe::maidsafe_error& exception) {
    std::cout << "Error: " << exception.what() << std::endl;
    return -2;
  }
  catch(...) {
    std::cout << "Unexpected exception." << std::endl;
    return -3;
  }

  return 0;
}
