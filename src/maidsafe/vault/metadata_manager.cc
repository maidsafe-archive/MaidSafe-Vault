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

#include "maidsafe/vault/metadata_manager.h"

#include <vector>

namespace maidsafe {

namespace vault {

MetadataManager::MetadataManager(routing::Routing& routing,
                                 const boost::filesystem::path& /*vault_root_dir*/)
    : routing_(routing) {
}

MetadataManager::~MetadataManager() {}

void MetadataManager::CloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
}

void MetadataManager::Serialise() {}



template<typename Data>
void MetadataManager::HandlePutMessage(const nfs::Message& /*message*/,
                                       const routing::ReplyFunctor& /*reply_functor*/) {}

template<typename Data>
void MetadataManager::HandlePostMessage(const nfs::Message& /*message*/,
                                        const routing::ReplyFunctor& /*reply_functor*/) {}

template<typename Data>
void MetadataManager::HandleDeleteMessage(const nfs::Message& /*message*/,
                                          const routing::ReplyFunctor& /*reply_functor*/) {}

void MetadataManager::SendSyncData() {}

bool MetadataManager::HandleNodeDown(NodeId& /*node*/) {
  return false;
}

bool MetadataManager::HandleNodeUp(NodeId& /*node*/) {
  return false;
}

// On error handler
template<typename Data>
void MetadataManager::OnPutErrorHandler(nfs::Message message) {}

template<typename Data>
void MetadataManager::OnDeleteErrorHandler(nfs::Message message) {}

}  // namespace vault

}  // namespace maidsafe
