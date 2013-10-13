/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_

#include <mutex>
#include <set>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/action_delete.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/vault/pmid_manager/handler.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {
namespace vault {

class PmidManagerService {
 public:
  typedef void PublicMessages;
  typedef PmidManagerServiceMessages VaultMessages;
  typedef PmidManagerServiceMessages Messages;

  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing);

  template <typename T>
  void HandleMessage(const T& message, const typename T::Sender& sender,
                     const typename T::Receiver& receiver);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}

  template <typename T>
  bool ValidateSender(const T& /*message*/, const typename T::Sender& /*sender*/) const {
    return false;
  }
  template <typename Data>
  void HandlePutResponse(const typename Data::Name& data_name, int32_t size,
      const PmidName& pmid_node, nfs::MessageId message_id);

  void HandleGetHealth(const PmidName& pmid_node, const MaidName& maid_node);

 private:
  PmidManagerService(const PmidManagerService&);
  PmidManagerService& operator=(const PmidManagerService&);
  PmidManagerService(PmidManagerService&&);
  PmidManagerService& operator=(PmidManagerService&&);

  // =============== Put/Delete data =============================================================
  template <typename Data>
  void HandlePut(const Data& data, const PmidName& pmid_node, nfs::MessageId message_id);

  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data,
                        const PmidName& pmid_node,
                        int64_t available_space,
                        const maidsafe_error& error_code,
                        nfs::MessageId message_id);

  template <typename Data>
  void HandleDelete(const PmidName& pmid_node, const typename Data::Name& data_name,
                    nfs::MessageId message_id);

  void DoSync();
  void SendPutResponse(const DataNameVariant& data_name, const PmidName& pmid_node, int32_t size,
                       nfs::MessageId message_id);

  void HandleSendPmidAccount(const PmidName& pmid_node, int64_t available_size);

  routing::Routing& routing_;
  GroupDb<PmidManager> group_db_;
  std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  PmidManagerDispatcher dispatcher_;
  std::map<PmidName, PmidManagerMetadata> pmid_metadata_;
  Sync<PmidManager::UnresolvedPut> sync_puts_;
  Sync<PmidManager::UnresolvedDelete> sync_deletes_;
  Sync<PmidManager::UnresolvedSetAvailableSize> sync_set_available_sizes_;
};

// ============================= Handle Message Specialisations ===================================

template <typename T>
void PmidManagerService::HandleMessage(const T& /*message*/, const typename T::Sender& /*sender*/,
                                       const typename T::Receiver& /*receiver*/) {
  // T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
}

template <>
void PmidManagerService::HandleMessage(
    const PutRequestFromDataManagerToPmidManager& message,
    const typename PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename PutRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const PutFailureFromPmidNodeToPmidManager& message,
    const typename PutFailureFromPmidNodeToPmidManager::Sender& sender,
    const typename PutFailureFromPmidNodeToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const DeleteRequestFromDataManagerToPmidManager& message,
    const typename DeleteRequestFromDataManagerToPmidManager::Sender& sender,
    const typename DeleteRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const GetPmidAccountRequestFromPmidNodeToPmidManager& message,
    const typename GetPmidAccountRequestFromPmidNodeToPmidManager::Sender& sender,
    const typename GetPmidAccountRequestFromPmidNodeToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const PmidHealthRequestFromMaidNodeToPmidManager& message,
    const typename PmidHealthRequestFromMaidNodeToPmidManager::Sender& sender,
    const typename PmidHealthRequestFromMaidNodeToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const SynchroniseFromPmidManagerToPmidManager& message,
    const typename SynchroniseFromPmidManagerToPmidManager::Sender& sender,
    const typename SynchroniseFromPmidManagerToPmidManager::Receiver& receiver);

// ==================== Implementation =============================================================

namespace detail {

template <typename PmidManagerSyncType>
void IncrementAttemptsAndSendSync(PmidManagerDispatcher& dispatcher,
                                  PmidManagerSyncType& sync_type) {
  auto unresolved_actions(sync_type.GetUnresolvedActions());
  if (!unresolved_actions.empty()) {
    sync_type.IncrementSyncAttempts();
    for (const auto& unresolved_action : unresolved_actions)
      dispatcher.SendSync(unresolved_action->key.group_name(), unresolved_action->Serialise());
    }
  }
}  // namespace detail

// ================================= Put Implementation ===========================================

template <typename Data>
void PmidManagerService::HandlePut(const Data& data, const PmidName& pmid_node,
                                   nfs::MessageId message_id) {
  dispatcher_.SendPutRequest(data, pmid_node, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), data.name().raw_name,
                             data.name().type);
  sync_puts_.AddLocalAction(
      PmidManager::UnresolvedPut(group_key,
                                 ActionPmidManagerPut(data.data().string().size(), message_id),
                                 routing_.kNodeId()));
  DoSync();
}

template <typename Data>
void PmidManagerService::HandlePutFailure(
    const typename Data::Name& name, const PmidName& pmid_node, int64_t available_space,
    const maidsafe_error& error_code, nfs::MessageId message_id) {
  pmid_metadata_.at(pmid_node).claimed_available_size = available_space;
  dispatcher_.SendPutFailure<Data>(name, pmid_node, error_code, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), name.raw_name, name.type);
  sync_deletes_.AddLocalAction(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(),
                                                             routing_.kNodeId()));
  DoSync();
}

template <typename Data>
void PmidManagerService::HandlePutResponse(
    const typename Data::Name& data_name, int32_t size, const PmidName& pmid_node,
    nfs::MessageId message_id) {
  dispatcher_.SendPutResponse<Data>(data_name, size, pmid_node, message_id);
}

template <typename Data>
void PmidManagerService::HandleDelete(
    const PmidName& pmid_node, const typename Data::Name& data_name,
    nfs::MessageId message_id) {
  dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), data_name.name().raw_name,
                             data_name.name().type);
  sync_deletes_.AddLocalAction(
      PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(), routing_.kNodeId()));
  DoSync();
}

// ===============================================================================================

}  // namespace vault
}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
