/***************************************************************************************************
 *  Copyright 2012 PmidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of PmidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of PmidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "maidsafe/routing/api_config.h"
//#include "maidsafe/nfs/network_file_system.h"
//#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/common/rsa.h"

namespace maidsafe {

namespace routing { class Routing; }
namespace nfs { class Message; }

namespace vault {

class PmidAccountHolder {
 public:
  PmidAccountHolder(routing::Routing& routing, const boost::filesystem::path vault_root_dir);
  ~PmidAccountHolder();
  template <typename Data>
  void HandleMessage(const nfs::Message& message,
                     const routing::ReplyFunctor& reply_functor);
 private:
  void HandlePutMessage(const nfs::Message& message);
  void HandleGetMessage(const nfs::Message& message);
  void HandlePostMessage(const nfs::Message& message);
  void HandleDeleteMessage(const nfs::Message& message);
  boost::filesystem::path vault_root_dir_;
//  routing::Routing& routing_;
//  DiskBasedStorage disk_storage_;
};

template <typename Data>
void PmidAccountHolder::HandleMessage(const nfs::Message& /*message*/,
                                      const routing::ReplyFunctor& /*reply_functor*/) {
}

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_H_
