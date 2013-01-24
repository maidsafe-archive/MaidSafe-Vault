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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_H_

#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/accumulator.h"

#include "maidsafe/vault/metadata_handler.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class MetadataManager {
 public:
  MetadataManager(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);
  ~MetadataManager();
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void Serialise();
  void CloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);

 private:
  template<typename Data>
  void HandleGetMessage(nfs::DataMessage data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePutMessage(const nfs::DataMessage& data_message,
                        const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::DataMessage& data_message,
                           const routing::ReplyFunctor& reply_functor);
  void SendSyncData();

  // use nodeinfo as later we may extract/set rank
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  bool HandleNodeDown(const nfs::GenericMessage& generic_message, NodeId& node);
  bool HandleNodeUp(const nfs::GenericMessage& generic_message, NodeId& node);

  // On error handler
  template<typename Data>
  void OnPutErrorHandler(nfs::DataMessage data_message);
  template<typename Data>
  void OnDeleteErrorHandler(nfs::DataMessage data_message);
  template<typename Data>
  void OnGenericErrorHandler(nfs::GenericMessage message);

  const boost::filesystem::path kRootDir_;
  routing::Routing& routing_;
  DataElementsManager data_elements_manager_;
  MetadataManagerNfs nfs_;
  nfs::Accumulator request_accumulator_;
};

template<typename Data>
void MetadataManager::HandleDataMessage(const nfs::DataMessage& /*data_message*/,
                                        const routing::ReplyFunctor& /*reply_functor*/) {
}

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/metadata_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_H_
