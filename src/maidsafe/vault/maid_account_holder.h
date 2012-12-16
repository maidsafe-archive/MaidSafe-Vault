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

#ifndef MAIDSAFE_VAULT_VAULT_H_
#define MAIDSAFE_VAULT_VAULT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"

#include "maidsafe/pd/client/node.h"

namespace maidsafe {

namespace vault {

class MaidAccountHolder {
 public:
  MaidAccountHolder(routing::Routing& routing, boost::filesystem::path vault_root_dir);
  void HandleMessage(const Message& message);

 private:
  bool SavePmidDataToDisk(Pmid);
  std::vector<Identity> ReadPmidDataFromDisk(Pmid);
  bool checkMessageSignature(Message& message);
  std::vector<Identity> pmid_data_elements_stored_;
  boost::filesystem::path vault_root_dir_;

};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
