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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_

#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/structured_data_manager/structured_data_key.h"
#include "maidsafe/vault/structured_data_manager/structured_data_value.h"
#include "maidsafe/vault/structured_data_manager/structured_data_merge_policy.h"
#include "maidsafe/vault/manager_db.h"


namespace maidsafe {

namespace vault {

class StructuredDataManagerService {
 public:
  typedef Identity StructuredDataAccountName;
  StructuredDataManagerService(const passport::Pmid& pmid,
                               routing::Routing& routing,
                               const boost::filesystem::path& path);
  template<typename Data>
  void HandleMessage(const nfs::Message& message,
                     const routing::ReplyFunctor& reply_functor);
  void HandleChurnEvent(routing::MatrixChange /*matrix_change*/) {}

 private:
  StructuredDataManagerService(const StructuredDataManagerService&);
  StructuredDataManagerService& operator=(const StructuredDataManagerService&);
  StructuredDataManagerService(StructuredDataManagerService&&);
  StructuredDataManagerService& operator=(StructuredDataManagerService&&);

  void ValidateClientSender(const nfs::Message& message) const;
  void ValidateSyncSender(const nfs::Message& message) const;
  std::vector<StructuredDataVersions::VersionName>
                       GetVersionsFromMessage(const nfs::Message& msg) const;
  //// =============== Get data ====================================================================

  void HandleGet(const nfs::Message& message, routing::ReplyFunctor reply_functor);
  void HandleGetBranch(const nfs::Message& message, routing::ReplyFunctor reply_functor);

  //// =============== Sync ========================================================================
  template<typename Data>
  void Syncronise(const nfs::Message& message);
  void HandleSyncronise(const nfs::Message& message);

  //// =============== Churn ============================================================
  void HandleChurnEvent(const NodeId& old_node, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::Message& message);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  std::mutex sync_mutex_;
  Accumulator<StructuredDataAccountName> accumulator_;
  ManagerDb<StructuredDataManager> structured_data_db_;
  const NodeId kThisNodeId_;
  Sync<StructuredDataMergePolicy> sync_;
  StructuredDataManagerNfs nfs_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/structured_data_manager/structured_data_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_
