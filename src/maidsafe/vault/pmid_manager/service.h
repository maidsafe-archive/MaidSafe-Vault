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
#include <map>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/timer.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/action_delete.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/operation_visitors.h"

namespace maidsafe {
namespace vault {

namespace test {

class PmidManagerServiceTest;

}

class PmidManagerService {
 public:
  using PublicMessages = void;
  using VaultMessages = PmidManagerServiceMessages;
  using Messages = PmidManagerServiceMessages;
  using HandleMessageReturnType = void;
  using AccountType = PmidManager::KvPair;

  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing);

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
  void SendPutResponse(const typename Data::Name& data_name, const PmidName& pmid_node,
                       nfs::MessageId message_id);

  void HandleCreatePmidAccountRequest(const PmidName& pmid_node, const MaidName& maid_node,
                                      nfs::MessageId message_id);
  template <typename Data>
  void HandleFalseNotification(const typename Data::Name& name, const PmidName& pmid_node,
                               int32_t size, nfs::MessageId message_id);

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
                        uint64_t size,
                        int64_t available_space,
                        const maidsafe_error& error_code,
                        nfs::MessageId message_id);

  template <typename Data>
  void HandleDelete(const PmidName& pmid_node, const typename Data::Name& data_name,
                    int32_t size, nfs::MessageId message_id);

  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);
  void SendPutResponse(const DataNameVariant& data_name, const PmidName& pmid_node,
                       nfs::MessageId message_id);

  void HandleSyncedPut(std::unique_ptr<PmidManager::UnresolvedPut>&& synced_action);
  void HandleSyncedDelete(std::unique_ptr<PmidManager::UnresolvedDelete>&& synced_action);
  void HandleSyncedCreatePmidAccount(
      std::unique_ptr<PmidManager::UnresolvedCreateAccount>&& synced_action);
  void HandleSyncedUpdateAccount(
      std::unique_ptr<PmidManager::UnresolvedUpdateAccount>&& synced_action);
  void TransferAccount(const NodeId& dest, const std::vector<PmidManager::KvPair>& accounts);
  void HandleAccountTransfer(const AccountType& account);
  void HandleAccountTransferEntry(const std::string& serialised_account,
                                  const routing::SingleSource& sender);
  void HandleAccountQuery(const PmidManager::Key& key, const NodeId& sender);
  void HandleUpdateAccount(const PmidName& pmid_node, int64_t diff_size);

  routing::Routing& routing_;
  std::map<PmidManager::Key, PmidManager::Value> accounts_;
  std::mutex accumulator_mutex_, mutex_;
  bool stopped_;
  Accumulator<Messages> accumulator_;
  PmidManagerDispatcher dispatcher_;
  AsioService asio_service_;
  routing::Timer<PmidManagerValue> get_health_timer_;
  Sync<PmidManager::UnresolvedPut> sync_puts_;
  Sync<PmidManager::UnresolvedDelete> sync_deletes_;
  Sync<PmidManager::UnresolvedCreateAccount> sync_create_account_;
  Sync<PmidManager::UnresolvedUpdateAccount> sync_update_account_;
  AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kPmidManager>> account_transfer_;
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

template<>
void PmidManagerService::HandleMessage(
    const AccountQueryFromPmidManagerToPmidManager& message,
    const typename AccountQueryFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountQueryFromPmidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const AccountQueryResponseFromPmidManagerToPmidManager& message,
    const typename AccountQueryResponseFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountQueryResponseFromPmidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const UpdateAccountFromDataManagerToPmidManager& message,
    const typename UpdateAccountFromDataManagerToPmidManager::Sender& sender,
    const typename UpdateAccountFromDataManagerToPmidManager::Receiver& receiver);

// =================================== Implementation =============================================
// ================================= Put Implementation ===========================================

template <typename Data>
void PmidManagerService::HandlePut(const Data& data, const PmidName& pmid_node,
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandlePut put request for chunk "
                <<  HexSubstr(data.name().value)
                << " to pmid_node -- " << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
  dispatcher_.SendPutRequest(data, pmid_node, message_id);
  PmidManager::SyncKey group_key(PmidManager::Key(pmid_node), data.name().value,
                                 Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedPut(group_key,
      ActionPmidManagerPut(static_cast<uint32_t>(data.Serialise().data.string().size()),
          message_id), routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::HandlePutFailure(
    const typename Data::Name& name, const PmidName& pmid_node, uint64_t size,
    int64_t available_space, const maidsafe_error& error_code, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandlePutFailure to pmid_node -- "
                << HexSubstr(pmid_node.value.string()) << " of size " << size
                << " , with message_id -- " << message_id.data
                << " . available_space -- " << available_space << " , error_code -- "
                << boost::diagnostic_information(error_code);
  dispatcher_.SendPutFailure<Data>(name, size, pmid_node, error_code, message_id);
  PmidManager::SyncKey group_key(PmidManager::Key(pmid_node), name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(size, false, true),
                                       routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::HandleFalseNotification(const typename Data::Name& name,
    const PmidName& pmid_node, int32_t size, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleFlaseNotification regarding pmid_node -- "
                << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
  PmidManager::SyncKey group_key(PmidManager::Key(pmid_node), name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(size, true, true),
                                       routing_.kNodeId()));
}

template <typename Data>
void PmidManagerService::SendPutResponse(
  const typename Data::Name& data_name, const PmidName& pmid_node, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::SendPutResponse of pmid_name -- "
                << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
  dispatcher_.SendPutResponse<Data>(data_name, pmid_node, message_id);
}

template <typename Data>
void PmidManagerService::HandleDelete(
    const PmidName& pmid_name, const typename Data::Name& data_name,
    int32_t size, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleDelete of " << HexSubstr(data_name.value)
                << " on pmid_node " << HexSubstr(pmid_name.value.string())
                << " , with message_id -- " << message_id.data;
  dispatcher_.SendDeleteRequest<Data>(pmid_name, data_name, message_id);
  PmidManager::SyncKey group_key(typename PmidManager::Key(pmid_name),
                                 data_name.value, Data::Tag::kValue);
  DoSync(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(size, true, false),
                                       routing_.kNodeId()));
}

// =============== Sync ============================================================================

template <typename UnresolvedAction>
void PmidManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_update_account_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_account_, unresolved_action);
}

// ===============================================================================================

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
