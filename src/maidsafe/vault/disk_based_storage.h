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

#ifndef MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
#define MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsfe/vault/disk_based_storage.pb.h"

#include "maidsafe/common/rsa.h"
#include "maidsafe/common/active.h"


namespace maidsafe {

namespace vault {

class DiskBasedContainer {
 public:
  DiskBasedContainer(boost::fileystem::path root_dir, Identity name);
  ~DiskBasedContainer();
  void Add(Identity name);
  void remove(Identity name);
  unit64_t Size();  // return total size of all data elements
 private:
  bool Find();
  Active active_;  // async writes ?
  // either write all saves assuming unique or
  // refund user in real time as we find out otherwise (i.e.
  // check for unique on insert asynchronously 
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
