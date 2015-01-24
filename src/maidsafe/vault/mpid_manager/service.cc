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
      sync_delete_messages_(NodeId(pmid.name()->string())) {}

MpidManagerService::~MpidManagerService() {}

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
    const nfs::GetRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::GetRequestFromMpidNodeToMpidManager;
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
        ImmutableData data(NonEmptyString(resolved_action->action.kMessage.Serialise()));
         handler_.Put(data, resolved_action->action.kMessage.base.sender);
        dispatcher_.SendMessageAlert(
            nfs_vault::MpidMessageAlert(resolved_action->action.kMessage.base,
                                        nfs_vault::MessageIdType(data.name().value.string())),
            resolved_action->action.kMessage.base.receiver, message.id);
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
  // After sync alert must be sent out -- TO BE IMPLEMENTED
  DoSync(MpidManager::UnresolvedPutMessage(MpidManager::SyncGroupKey(sender),
                                           ActionMpidManagerPutMessage(message),
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
