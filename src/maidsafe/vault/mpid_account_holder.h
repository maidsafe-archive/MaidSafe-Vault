/***************************************************************************************************
 *  Copyright 2012 MpidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MpidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MpidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_H_

#include <string>
#include <vector>

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/network_file_system.h"
#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/common/rsa.h"

namespace maidsafe {

namespace nfs { class Message; }

namespace vault {

class MpidAccountHolder {
 public:
  MpidAccountHolder(routing::Routing& routing, const boost::filesystem::path vault_root_dir);
  ~MpidAccountHolder();
  void HandleMessage(const nfs::Message& message);
 private:
  void HandlePutMessage(const Message& message);
  void HandleGetMessage(const Message& message);
  void HandlePostMessage(const Message& message);
  void HandleDeleteMessage(const Message& message);
  boost::filesystem::path vault_root_dir_;
  routing::Routing& routing_;
  DiskBasedStorage disk_storage_;
};

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_H_
