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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_H_

#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/data_elements_manager.h"
#include "maidsafe/nfs/maid_account.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/post_message.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/public_key_getter.h"

namespace maidsafe {

namespace vault {

class MetadataManager {
 public:
  MetadataManager(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);
  ~MetadataManager();
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void Serialise();

 private:
  template<typename Data>
  void HandleGetMessage(nfs::Message message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePostMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void SendSyncData();

  // use nodeinfo as later we may extract/set rank
  bool HandleNodeDown(const nfs::PostMessage& message, NodeId& node);
  bool HandleNodeUp(const nfs::PostMessage& message, NodeId& node);

  // On error handler
  template<typename Data>
  void OnPutErrorHandler(nfs::Message message);
  template<typename Data>
  void OnDeleteErrorHandler(nfs::Message message);

  const boost::filesystem::path kRootDir_;
  routing::Routing& routing_;
  nfs::DataElementsManager data_elements_manager_;
//  nfs::MetadataManagerNfs nfs_;
};

template<typename Data>
void MetadataManager::HandleMessage(const nfs::Message& /*message*/,
                                    const routing::ReplyFunctor& /*reply_functor*/) {
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_H_
