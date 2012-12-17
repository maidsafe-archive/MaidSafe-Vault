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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"

namespace maidsafe {

namespace vault {

// will confirm signature matches src ID private key
// signed the type (three enums) and payload. Will do a Get from MM
bool checkMessageSignature(Message& message);

// this can be pmid lists for a maid
// data stored for a maid
// data stored on a pmid
// data hodlers for a MM
class DiskBasedStorage {
  DiskBasedSorage(boost::filesystem::path name);
  bool Save(Identity name);
  bool Find(Identity name);
  bool Delete(Identity name);
  std::vector<Identity> ReadAll();
 private:
  boost::filesystem::path name_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
