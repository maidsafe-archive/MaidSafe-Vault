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

#ifndef MAIDSAFE_VAULT_DATA_HOLDER_H_
#define MAIDSAFE_VAULT_DATA_HOLDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "boost/filesystem/path.hpp"
#include <boost/graph/graph_concepts.hpp>

#include "maidsafe/common/rsa.h"

#include "maidsafe/routing/api_config.h"


#include "maidsafe/vault/get_policies.h"
#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/delete_policies.h"
#include "maidsafe/vault/put_policies.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/private/data_store.h"


#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace routing { class Routing; }
namespace nfs { class Message; }

namespace vault {

typedef Nfs<NoGet, NoPut, NoPost, NoDelete> DataHolderNfs;

class DataHolder {
 public:
  DataHolder(routing::Routing& routing, const boost::filesystem::path vault_root_dir);
  ~DataHolder();
  void HandleMessage(const nfs::Message& message, routing::ReplyFunctor reply_functor);
  bool HaveCache(nfs::Message& message);
  bool HaveCache(nfs::Message& message,
                 const routing::Routing& routing,
                 const DiskBasedStorage& disk_storage);
  void StoreCache(const nfs::Message& message);
  void StoreCache(const nfs::Message& message,
                  const routing::Routing& routing,
                  const DiskBasedStorage& disk_storage);
  void StopSending();

 private:
  void HandlePutMessage(const nfs::Message& message);
  void HandleGetMessage(const nfs::Message& message);
  void HandlePostMessage(const nfs::Message& message);
  void HandleDeleteMessage(const nfs::Message& message);
  boost::filesystem::path persona_dir_;
  routing::Routing& routing_;
  DataStore dara_store_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_H_
