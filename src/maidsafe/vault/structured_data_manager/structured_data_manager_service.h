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
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class StructuredDataManagerService {
 public:
  typedef std::pair<Identity, Identity> AccountName;
  StructuredDataManagerService(const passport::Pmid& /*pmid*/,
                               routing::Routing& /*routing*/,
                               nfs::PublicKeyGetter& /*public_key_getter*/,
                               Db& /*db*/) {}
  // Handling of received requests (sending of requests is done via nfs_ object).
  template<typename Data>
  void HandleMessage(const nfs::Message& /*message*/, const routing::ReplyFunctor& /*reply_functor*/) {}
  void HandleChurnEvent(routing::MatrixChange /*matrix_change*/) {}

 private:
  StructuredDataManagerService(const StructuredDataManagerService&);
  StructuredDataManagerService& operator=(const StructuredDataManagerService&);
  StructuredDataManagerService(StructuredDataManagerService&&);
  StructuredDataManagerService& operator=(StructuredDataManagerService&&);

  //void ValidateDataSender(const nfs::Message& message) const;
  //void ValidateGenericSender(const nfs::Message& message) const;

  //// =============== Put/Delete data ===============================================================
  //template<typename Data>
  //void HandlePut(const nfs::Message& message);
  //template<typename Data>
  //void HandleDelete(const nfs::Message& message);
  //template<typename Data>
  //void HandleGet(const nfs::Message& message, routing::ReplyFunctor reply_functor);

  //template<typename Data, nfs::MessageAction action>
  //void AddLocalUnresolvedEntryThenSync(const nfs::Message& message, int32_t cost);

  //// =============== Sync ==========================================================================
  //void Sync(const MaidName& account_name);
  //void HandleSync(const nfs::Message& message);

  //// =============== Account transfer ==============================================================
  //void TransferAccount(const MaidName& account_name, const NodeId& new_node);
  //void HandleAccountTransfer(const nfs::Message& message);

  //routing::Routing& routing_;
  //nfs::PublicKeyGetter& public_key_getter_;
  //std::mutex accumulator_mutex_;
  //AccountName account_name_;
  //Accumulator<AccountName> accumulator_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/structured_data_manager/structured_data_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_
