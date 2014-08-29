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
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/timer.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/account_transfer.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/action_delete.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/operation_visitors.h"

namespace maidsafe {
namespace vault {

namespace test {

class PmidManagerServiceTest;

}

class PmidManagerService {
 public:
  typedef void PublicMessages;
  typedef PmidManagerServiceMessages VaultMessages;
  typedef PmidManagerServiceMessages Messages;
  typedef void HandleMessageReturnType;

  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     const boost::filesystem::path& vault_root_dir);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

  void Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
  }

  template <typename T>
  bool ValidateSender(const T& /*message*/, const typename T::Sender& /*sender*/) const {
    // BEFORE_RELEASE implementation missing
    return true;
  }
  template <typename Data>
  void SendPutResponse(const typename Data::Name& data_name, int32_t size,
                       const PmidName& pmid_node, nfs::MessageId message_id);

  void HandleCreatePmidAccountRequest(const PmidName& pmid_node, const MaidName& maid_node,
                                      nfs::MessageId message_id);
  void HandleHealthRequest(const PmidName& pmid_node, const MaidName& maid_node,
                           nfs::MessageId message_id);
  void HandleHealthResponse(const PmidName& pmid_node, uint64_t available_size,
                            nfs::MessageId message_id);

  template <typename Data>
  void HandleFalseNotification(
      const typename Data::Name& name, const PmidName& pmid_node, nfs::MessageId message_id);

 private:
  PmidManagerService(const PmidManagerService&);
  PmidManagerService& operator=(const PmidManagerService&);
  PmidManagerService(PmidManagerService&&);
  PmidManagerService& operator=(PmidManagerService&&);

  template<typename ServiceHandlerType, typename MessageType>
  friend void detail::DoOperation(
      ServiceHandlerType* service, const MessageType& message,
      const typename MessageType::Sender& sender,
      const typename MessageType::Receiver& receiver);

  friend class detail::PmidManagerPutVisitor<PmidManagerService>;
  friend class detail::PmidManagerPutResponseFailureVisitor<PmidManagerService>;
  friend class detail::PmidManagerDeleteVisitor<PmidManagerService>;
  friend class test::PmidManagerServiceTest;

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

  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);
  void SendPutResponse(const DataNameVariant& data_name, const PmidName& pmid_node, int32_t size,
                       nfs::MessageId message_id);

  void HandleSendPmidAccount(const PmidName& pmid_node, int64_t available_size);

  void HandleSyncedPut(std::unique_ptr<PmidManager::UnresolvedPut>&& synced_action);
  void HandleSyncedDelete(std::unique_ptr<PmidManager::UnresolvedDelete>&& synced_action);
  void HandleSyncedSetPmidHealth(
      std::unique_ptr<PmidManager::UnresolvedSetPmidHealth>&& synced_action);
  void HandleSyncedCreatePmidAccount(
      std::unique_ptr<PmidManager::UnresolvedCreateAccount>&& synced_action);

  void DoHandleHealthResponse(const PmidName& pmid_node,
      const MaidName& maid_node, const PmidManagerMetadata& pmid_health, nfs::MessageId message_id);

  void TransferAccount(const NodeId& dest,
                       const std::vector<GroupDb<PmidManager>::Contents>& accounts);

  void HandleAccountTransfer(
      std::unique_ptr<PmidManager::UnresolvedAccountTransfer>&& resolved_action);

  routing::Routing& routing_;
  GroupDb<PmidManager> group_db_;
  std::mutex accumulator_mutex_, mutex_;
  bool stopped_;
  Accumulator<Messages> accumulator_;
  PmidManagerDispatcher dispatcher_;
  AsioService asio_service_;
  routing::Timer<PmidManagerMetadata> get_health_timer_;
  Sync<PmidManager::UnresolvedPut> sync_puts_;
  Sync<PmidManager::UnresolvedDelete> sync_deletes_;
  Sync<PmidManager::UnresolvedSetPmidHealth> sync_set_pmid_health_;
  Sync<PmidManager::UnresolvedCreateAccount> sync_create_account_;
  AccountTransfer<PmidManager::UnresolvedAccountTransfer> account_transfer_;
};

// ============================= Handle Message Specialisations ===================================
template <typename MessageType>
void PmidManagerService::HandleMessage(const MessageType& /*message*/,
                                       const typename MessageType::Sender& /*sender*/,
                                       const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_genereic_handler_is_available__Specialisation_is_required;
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
    const PmidHealthRequestFromMaidManagerToPmidManager& message,
    const typename PmidHealthRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename PmidHealthRequestFromMaidManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const PmidHealthResponseFromPmidNodeToPmidManager& message,
    const typename PmidHealthResponseFromPmidNodeToPmidManager::Sender& sender,
    const typename PmidHealthResponseFromPmidNodeToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const CreatePmidAccountRequestFromMaidManagerToPmidManager& message,
    const typename CreatePmidAccountRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename CreatePmidAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const IntegrityCheckRequestFromDataManagerToPmidManager& message,
    const typename IntegrityCheckRequestFromDataManagerToPmidManager::Sender& sender,
    const typename IntegrityCheckRequestFromDataManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const SynchroniseFromPmidManagerToPmidManager& message,
    const typename SynchroniseFromPmidManagerToPmidManager::Sender& sender,
    const typename SynchroniseFromPmidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const AccountTransferFromPmidManagerToPmidManager& message,
    const typename AccountTransferFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountTransferFromPmidManagerToPmidManager::Receiver& receiver);

// ==================== Implementation =============================================================


// ================================= Put Implementation ===========================================

template <typename Data>
void PmidManagerService::HandlePut(const Data& data, const PmidName& pmid_node,
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandlePut put request for chunk "
                <<  HexSubstr(data.name().value)
                << " to pmid_node -- " << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
//   try {
//     PmidManagerMetadata reply(group_db_.GetContents(pmid_node).metadata);
//     if (reply.claimed_available_size <
//         static_cast<uint32_t>(data.Serialise().data.string().size())) {
//       dispatcher_.SendPutFailure<Data>(data.name(), pmid_node,
//                                        maidsafe_error(VaultErrors::not_enough_space),
//                                        message_id);
//       return;
//     }
//   } catch(...) {
//   }
  dispatcher_.SendPutRequest(data, pmid_node, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), data.name().value,
                             Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedPut(group_key,
      ActionPmidManagerPut(static_cast<uint32_t>(data.Serialise().data.string().size()),
          message_id), routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::HandlePutFailure(
    const typename Data::Name& name, const PmidName& pmid_node, int64_t available_space,
    const maidsafe_error& error_code, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandlePutFailure to pmid_node -- "
                << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data
                << " . available_space -- " << available_space << " , error_code -- "
                << boost::diagnostic_information(error_code);
  dispatcher_.SendPutFailure<Data>(name, pmid_node, error_code, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(false, true),
                                       routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::HandleFalseNotification(
    const typename Data::Name& name, const PmidName& pmid_node, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleFlaseNotification regarding pmid_node -- "
                << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(true, true),
                                       routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::SendPutResponse(
    const typename Data::Name& data_name, int32_t size, const PmidName& pmid_name,
    nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::SendPutResponse of pmid_name -- "
                << HexSubstr(pmid_name.value.string())
                << " , with message_id -- " << message_id.data
                << " . size -- " << size;
  dispatcher_.SendPutResponse<Data>(data_name, size, pmid_name, message_id);
}

template <typename Data>
void PmidManagerService::HandleDelete(
    const PmidName& pmid_name, const typename Data::Name& data_name,
    nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleDelete of " << HexSubstr(data_name.value)
                << " on pmid_node " << HexSubstr(pmid_name.value.string())
                << " , with message_id -- " << message_id.data;
  dispatcher_.SendDeleteRequest<Data>(pmid_name, data_name, message_id);
  PmidManager::Key group_key(typename PmidManager::GroupName(pmid_name),
                             data_name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(true, false),
                                       routing_.kNodeId()));
}

// =============== Sync ============================================================================

template <typename UnresolvedAction>
void PmidManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_set_pmid_health_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_account_, unresolved_action);
}

// ===============================================================================================

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
