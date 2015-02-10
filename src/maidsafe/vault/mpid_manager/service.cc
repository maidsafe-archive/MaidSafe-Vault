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

#include <string>

#include "maidsafe/vault/mpid_manager/service.h"
#include "maidsafe/vault/operation_handlers.h"

namespace maidsafe {

namespace vault {

MpidManagerService::MpidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       const boost::filesystem::path& vault_root_dir,
                                       DiskUsage max_disk_usage)
    : routing_(routing),
      accumulator_mutex_(),
      nodes_change_mutex_(),
      accumulator_(),
      close_nodes_change_(),
      client_nodes_change_(),
      dispatcher_(routing),
      handler_(vault_root_dir, max_disk_usage),
//      account_transfer_(),
      sync_put_alerts_(NodeId(pmid.name()->string())),
      sync_delete_alerts_(NodeId(pmid.name()->string())),
      sync_put_messages_(NodeId(pmid.name()->string())),
      sync_delete_messages_(NodeId(pmid.name()->string())),
      sync_create_accounts_(NodeId(pmid.name()->string())),
      sync_remove_accounts_(NodeId(pmid.name()->string())) {}

MpidManagerService::~MpidManagerService() {}

template <>
void MpidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMpidNodeToMpidManager& message,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using MessageType = nfs::CreateAccountRequestFromMpidNodeToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(const SendAlertFromMpidManagerToMpidManager &message,
    const typename SendAlertFromMpidManagerToMpidManager::Sender& sender,
    const typename SendAlertFromMpidManagerToMpidManager::Receiver& receiver) {
  using MessageType = SendAlertFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const nfs::GetMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::GetMessageRequestFromMpidNodeToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const GetRequestFromMpidManagerToMpidManager& message,
    const typename GetRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename GetRequestFromMpidManagerToMpidManager::Receiver& receiver) {
  using  MessageType = GetRequestFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const GetResponseFromMpidManagerToMpidManager& message,
    const typename GetResponseFromMpidManagerToMpidManager::Sender& sender,
    const typename GetResponseFromMpidManagerToMpidManager::Receiver& receiver) {
  using  MessageType = GetResponseFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMpidNodeToMpidManager& message,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::DeleteRequestFromMpidNodeToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const DeleteRequestFromMpidManagerToMpidManager& message,
    const typename DeleteRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename DeleteRequestFromMpidManagerToMpidManager::Receiver& receiver) {
  using  MessageType = DeleteRequestFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const nfs::SendMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::SendMessageRequestFromMpidNodeToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const PutResponseFromDataManagerToMpidManager& message,
    const typename PutResponseFromDataManagerToMpidManager::Sender& sender,
    const typename PutResponseFromDataManagerToMpidManager::Receiver& receiver) {
  using  MessageType = PutResponseFromDataManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// =============== Mpid Account Creation ===========================================================

void MpidManagerService::HandleCreateAccount(const passport::PublicMpid& public_mpid,
                                             const passport::PublicAnmpid& public_anmpid,
                                             nfs::MessageId message_id) {
  nfs::MessageId mpid_hash_message_id(vault::HashStringToMessageId(public_mpid.name()->string()));
  nfs::MessageId anmpid_hash_message_id(
                     vault::HashStringToMessageId(public_anmpid.name()->string()));
  nfs::MessageId mpid_message_id(mpid_hash_message_id ^ message_id);
  nfs::MessageId anmpid_message_id(anmpid_hash_message_id ^ message_id);

  if (handler_.HasAccount(public_mpid.name())) {
    maidsafe_error error(MakeError(VaultErrors::account_already_exists));
    dispatcher_.SendCreateAccountResponse(public_mpid.name(), error, message_id);
    return;
  }
  {
    std::lock_guard<std::mutex> lock(pending_account_mutex_);
    MpidAccountCreationStatus account_creation_status(public_mpid.name(), public_anmpid.name());
    pending_account_map_.insert(std::make_pair(message_id, account_creation_status));
  }

  dispatcher_.SendPutRequest(public_mpid.name(), public_mpid, mpid_message_id);
  dispatcher_.SendPutRequest(public_mpid.name(), public_anmpid, anmpid_message_id);
}

template <>
void MpidManagerService::HandlePutResponse<passport::PublicMpid>(
         const MpidName& mpid_name, const typename passport::PublicMpid::Name& data_name,
         int64_t /*size*/, nfs::MessageId mpid_message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  nfs::MessageId mpid_hash_message_id(vault::HashStringToMessageId(data_name->string()));
  nfs::MessageId original_message_id(mpid_message_id ^ mpid_hash_message_id);
  auto pending_account_itr(pending_account_map_.find(original_message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  // In case of a churn, drop it silently
  if (data_name != mpid_name)
    return;
  assert(pending_account_itr->second.name == data_name);
  assert(vault::HashStringToMessageId(pending_account_itr->second.name->string())
         == mpid_hash_message_id);
  static_cast<void>(data_name);
  pending_account_itr->second.name_stored = true;

  if (pending_account_itr->second.an_name_stored) {
    DoSync(MpidManager::UnresolvedCreateAccount(MpidManager::SyncGroupKey(mpid_name),
                                                ActionCreateAccount(original_message_id),
                                                routing_.kNodeId()));
    pending_account_map_.erase(pending_account_itr);
  }
}

template <>
void MpidManagerService::HandlePutResponse<passport::PublicAnmpid>(
         const MpidName& mpid_name, const typename passport::PublicAnmpid::Name& data_name,
         int64_t /*size*/, nfs::MessageId anmpid_message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  nfs::MessageId anmpid_hash_message_id(vault::HashStringToMessageId(data_name->string()));
  nfs::MessageId original_message_id(anmpid_message_id ^ anmpid_hash_message_id);
  auto pending_account_itr(pending_account_map_.find(original_message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  assert(pending_account_itr->second.an_name == data_name);
  assert(vault::HashStringToMessageId(pending_account_itr->second.an_name->string())
         == anmpid_hash_message_id);
  static_cast<void>(data_name);
  pending_account_itr->second.an_name_stored = true;

  if (pending_account_itr->second.name_stored) {
    DoSync(MpidManager::UnresolvedCreateAccount(MpidManager::SyncGroupKey(mpid_name),
                                                ActionCreateAccount(original_message_id),
                                                routing_.kNodeId()));
    pending_account_map_.erase(pending_account_itr);
  }
}

template <>
void MpidManagerService::HandlePutFailure<passport::PublicMpid>(
         const MpidName& mpid_name, const passport::PublicMpid::Name& data_name,
         const maidsafe_error& error, nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  nfs::MessageId mpid_hash_message_id(vault::HashStringToMessageId(data_name->string()));
  nfs::MessageId original_message_id(message_id ^ mpid_hash_message_id);
  auto pending_account_itr(pending_account_map_.find(original_message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  assert(data_name == mpid_name);
  assert(pending_account_itr->second.name == data_name);
  assert(vault::HashStringToMessageId(pending_account_itr->second.name->string())
         == message_id);
  static_cast<void>(data_name);
  // TODO(Team): Consider deleting anmpid key if stored
  pending_account_map_.erase(pending_account_itr);
  dispatcher_.SendCreateAccountResponse(mpid_name, error, original_message_id);
}

template <>
void MpidManagerService::HandlePutFailure<passport::PublicAnmpid>(
         const MpidName& mpid_name, const passport::PublicAnmpid::Name& data_name,
         const maidsafe_error& error, nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  nfs::MessageId anmpid_hash_message_id(vault::HashStringToMessageId(data_name->string()));
  nfs::MessageId original_message_id(message_id ^ anmpid_hash_message_id);
  auto pending_account_itr(pending_account_map_.find(original_message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  assert(pending_account_itr->second.an_name == data_name);
  assert(vault::HashStringToMessageId(pending_account_itr->second.an_name->string())
         == message_id);
  static_cast<void>(data_name);
  // TODO(Team): Consider deleting mpid key if stored
  pending_account_map_.erase(pending_account_itr);
  dispatcher_.SendCreateAccountResponse(mpid_name, error, original_message_id);
}

void MpidManagerService::HandleSyncedCreateAccount(
         std::unique_ptr<MpidManager::UnresolvedCreateAccount>&& synced_action) {
  maidsafe_error error(CommonErrors::success);
  dispatcher_.SendCreateAccountResponse(synced_action->key.group_name(), error,
                                        synced_action->action.kMessageId);
}

void MpidManagerService::HandleSyncedRemoveAccount(
         std::unique_ptr<MpidManager::UnresolvedRemoveAccount>&& synced_action) {
  dispatcher_.SendRemoveAccountResponse(synced_action->key.group_name(),
                                        maidsafe_error(CommonErrors::success),
                                        synced_action->action.kMessageId);
}

// ========================== Sync / AccountTransfer implementation ================================

template <>
void MpidManagerService::HandleMessage(
    const SynchroniseFromMpidManagerToMpidManager& message,
    const typename SynchroniseFromMpidManagerToMpidManager::Sender& sender,
    const typename SynchroniseFromMpidManagerToMpidManager::Receiver& /*receiver*/) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionMpidManagerPutMessage::kActionId: {
      MpidManager::UnresolvedPutMessage unresolved_action(proto_sync.serialised_unresolved_action(),
                                                          sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_put_messages_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        ImmutableData data(NonEmptyString(
                               resolved_action->action.kMessageAndId.message.Serialise()));
        handler_.Put(data, resolved_action->action.kMessageAndId.message.base.sender);
        dispatcher_.SendMessageAlert(
            nfs_vault::MpidMessageAlert(resolved_action->action.kMessageAndId.message.base,
                                        nfs_vault::MessageIdType(data.name().value.string())),
            resolved_action->action.kMessageAndId.message.base.receiver, message.id);
      }
      break;
    }
    case ActionMpidManagerDeleteMessage::kActionId: {
      MpidManager::UnresolvedDeleteMessage
          unresolved_action(proto_sync.serialised_unresolved_action(), sender.sender_id,
                            routing_.kNodeId());
      auto resolved_action(sync_delete_messages_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
       handler_.Delete(ImmutableData::Name(resolved_action->action.kAlert.message_id));
      }
      break;
    }
    case ActionMpidManagerPutAlert::kActionId: {
      MpidManager::UnresolvedPutAlert unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_put_alerts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        ImmutableData data(NonEmptyString(resolved_action->action.kAlert.Serialise()));
        handler_.Put(data, resolved_action->action.kAlert.base.receiver);
        if (IsOnline(resolved_action->key.group_name()))
          dispatcher_.SendMessageAlert(resolved_action->action.kAlert,
                                       resolved_action->key.group_name(), message.id);
      }
      break;
    }
    case ActionMpidManagerDeleteAlert::kActionId: {
      MpidManager::UnresolvedDeleteAlert unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_delete_alerts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        ImmutableData data(NonEmptyString(resolved_action->action.kAlert.Serialise()));
        handler_.Delete(data.name());
      }
      break;
    }
    case ActionCreateAccount::kActionId: {
      MpidManager::UnresolvedCreateAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_create_accounts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        HandleSyncedCreateAccount(std::move(resolved_action));
      }
      break;
    }
    case ActionRemoveAccount::kActionId: {
      MpidManager::UnresolvedRemoveAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_remove_accounts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        HandleSyncedRemoveAccount(std::move(resolved_action));
      }
      break;
    }
    default: {
      LOG(kError) << "SynchroniseFromMpidManagerToMpidManager Unhandled action type";
      assert(false && "Unhandled action type");
    }
  }
}

// ================================================================================================

void MpidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::ClientNodesChange> client_nodes_change) {
  std::lock_guard<decltype(nodes_change_mutex_)> lock(nodes_change_mutex_);
  client_nodes_change_ = *client_nodes_change;
}

void MpidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> /*close_nodes_change*/) {}


void MpidManagerService::HandleSendMessage(const nfs_vault::MpidMessage& message,
                                           const MpidName& sender, nfs::MessageId message_id) {
  if (!handler_.HasAccount(sender)) {
    dispatcher_.SendMessageResponse(sender, MakeError(VaultErrors::no_such_account), message_id);
    return;
  }
  dispatcher_.SendMessageResponse(sender, MakeError(CommonErrors::success), message_id);
  DoSync(MpidManager::UnresolvedPutMessage(MpidManager::SyncGroupKey(sender),
                                           ActionMpidManagerPutMessage(message, message_id),
                                           routing_.kNodeId()));
}

void MpidManagerService::HandleMessageAlert(const nfs_vault::MpidMessageAlert& alert,
                                            const MpidName& receiver) {
  if (!handler_.HasAccount(receiver))
    return;

  DoSync(MpidManager::UnresolvedPutAlert(
      MpidManager::SyncGroupKey(receiver), ActionMpidManagerPutAlert(alert), routing_.kNodeId()));
}

void MpidManagerService::HandleGetMessageRequestFromMpidNode(
    const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver, nfs::MessageId message_id) {
  ImmutableData data(NonEmptyString(alert.Serialise()));
  if (!handler_.Has(data.name()))
    return;

  dispatcher_.SendGetMessageRequest(alert, receiver, message_id);
}

void MpidManagerService::HandleGetMessageRequest(const nfs_vault::MpidMessageAlert& alert,
                                                 const MpidName& receiver,
                                                 nfs::MessageId message_id) {
  return dispatcher_.SendGetMessageResponse(
      handler_.GetMessage(ImmutableData::Name(alert.message_id)), alert.base.sender, receiver,
                          message_id);
}

void MpidManagerService::HandleGetMessageResponse(
    const nfs_client::MpidMessageOrReturnCode& response, const MpidName& receiver,
    nfs::MessageId message_id) {
  return dispatcher_.SendGetMessageResponseToMpid(response, receiver, message_id);
}

bool MpidManagerService::IsOnline(const MpidName& mpid_name) {
  std::vector<NodeId> new_clients;
  {
    std::lock_guard<decltype(nodes_change_mutex_)> lock(nodes_change_mutex_);
    new_clients = client_nodes_change_.new_close_nodes();
  }
  return std::any_of(new_clients.begin(), new_clients.end(),
                     [&](const NodeId& node_id) {
                       return NodeId(mpid_name->string()) == node_id;
                     });
}

void MpidManagerService::HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& receiver) {
  if (!handler_.Has(ImmutableData::Name(alert.message_id)))
    return;

  DoSync(MpidManager::UnresolvedDeleteAlert(MpidManager::SyncGroupKey(receiver),
                                            ActionMpidManagerDeleteAlert(alert),
                                            routing_.kNodeId()));

  dispatcher_.SendDeleteRequest(alert, receiver);
}

void MpidManagerService::HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& /*receiver*/,
                                             const MpidName& sender) {
  if (!handler_.HasAccount(alert.base.sender))
    return;

  auto expected(handler_.GetMessage(ImmutableData::Name(alert.message_id)));
  if (!expected.valid())
    return;

  DoSync(MpidManager::UnresolvedDeleteMessage(MpidManager::SyncGroupKey(sender),
                                              ActionMpidManagerDeleteMessage(alert),
                                              routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
