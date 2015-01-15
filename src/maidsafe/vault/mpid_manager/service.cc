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
                                       nfs_client::DataGetter& data_getter,
                                       const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      asio_service_(2),
      data_getter_(data_getter),
      accumulator_mutex_(),
      dispatcher_(routing),
      db_(vault_root_dir),
      account_transfer_(),
      sync_put_alerts_(NodeId(pmid.name()->string())),
      sync_delete_alerts_(NodeId(pmid.name()->string())),
      sync_put_messages_(NodeId(pmid.name()->string())),
      sync_delete_messages_(NodeId(pmid.name()->string())) {}

template <>
void MpidManagerService::HandleMessage(const MessageAlertFromMpidManagerToMpidManager &message,
    const typename MessageAlertFromMpidManagerToMpidManager::Sender& sender,
    const typename MessageAlertFromMpidManagerToMpidManager::Receiver& receiver) {
  using MessageType = MessageAlertFromMpidManagerToMpidManager;
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
    const nfs::SendMessageFromMpidNodeToMpidManager& message,
    const typename nfs::SendMessageFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::SendMessageFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::SendMessageFromMpidNodeToMpidManager;
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
    const typename SynchroniseFromMpidManagerToMpidManager::Sender& /*sender*/,
    const typename SynchroniseFromMpidManagerToMpidManager::Receiver& /*receiver*/) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
//    case ActionMpidManagerPutMessage::kActionId: {
//      MpidManager::UnresolvedPutMessage unresolved_action(proto_sync.serialised_unresolved_action(),
//                                                          sender.sender_id, routing_.kNodeId());
//      auto resolved_action(sync_put_messages_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action) {
//        db_.Commit(resolved_action->key, resolved_action->action);
//        dispatcher_.SendMessageAlert(
//            nfs_vault::MpidMessageAlert(resolved_action->key, resolved_action->action.kMessage.id,
//                                        resolved_action->action.kMessage.parent_id,
//                                        resolved_action->action.kMessage.signed_header)
//            , resolved_action->action.kMessage.receiver);
//      }
//      break;
//    }
//    case ActionDataManagerDelete::kActionId: {
//      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete";
//      DataManager::UnresolvedDelete unresolved_action(proto_sync.serialised_unresolved_action(),
//                                                      sender.sender_id, routing_.kNodeId());
//      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action) {
//        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete "
//                   << "resolved for chunk " << HexSubstr(resolved_action->key.name.string());
//        auto value(db_.Commit(resolved_action->key, resolved_action->action));
//        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete "
//                   << "the chunk " << HexSubstr(resolved_action->key.name.string());
//        if (value) {
//          // The delete operation will not depend on subscribers anymore.
//          // Owners' signatures may stored in DM later on to support deletes.
//          LOG(kInfo) << "SynchroniseFromDataManagerToDataManager send delete request";
//          std::set<PmidName> all_pmids_set;
//          auto all_pmids(value->AllPmids());
//          for (auto pmid : all_pmids)
//            all_pmids_set.insert(pmid);
//          SendDeleteRequests(resolved_action->key, all_pmids_set,
//                             resolved_action->action.MessageId());
//        }
//      }
//      break;
//    }
//    case ActionDataManagerAddPmid::kActionId: {
//      DataManager::UnresolvedAddPmid unresolved_action(
//          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
//      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerAddPmid "
//                    << " for chunk " << HexSubstr(unresolved_action.key.name.string())
//                    << " and pmid_node " << HexSubstr(unresolved_action.action.kPmidName->string());
//      auto resolved_action(sync_add_pmids_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action) {
//        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit add pmid to db"
//                   << " for chunk " << HexSubstr(unresolved_action.key.name.string())
//                   << " and pmid_node " << HexSubstr(unresolved_action.action.kPmidName->string());
//        try {
//          db_.Commit(resolved_action->key, resolved_action->action);
//        }
//        catch (const maidsafe_error& error) {
//          if (error.code() != make_error_code(CommonErrors::no_such_element))
//            throw;
//        }
//      }
//      break;
//    }
//    case ActionDataManagerRemovePmid::kActionId: {
//      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerRemovePmid";
//      DataManager::UnresolvedRemovePmid unresolved_action(
//          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
//      auto resolved_action(sync_remove_pmids_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action) {
//        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit remove pmid to db";
//        // The PmidManager pass down the PutFailure from PmidNode immediately after received it
//        // This may cause the sync_remove_pmid got resolved before the sync_add_pmid
//        // In that case, the commit will raise an error of no_such_account
//        // BEFORE_RELEASE double check whether the "mute" solution is enough
//        //                as the pmid_node will get added eventually and may cause problem for get
//        try {
//          db_.Commit(resolved_action->key, resolved_action->action);
//        } catch(maidsafe_error& error) {
//          LOG(kWarning) << "having error when trying to commit remove pmid to db : "
//                        << boost::diagnostic_information(error);
//        }
//      }
//      break;
//    }
    default: {
      LOG(kError) << "SynchroniseFromDataManagerToDataManager Unhandled action type";
      assert(false && "Unhandled action type");
    }
  }
}

// ================================================================================================


void MpidManagerService::HandleSendMessage(const nfs_vault::MpidMessage& message,
                                           const MpidName& sender) {
  if (!db_.Exists(sender)) {
    dispatcher_.SendMessageResponse(sender, MakeError(VaultErrors::no_such_account));
    return;
  }
  dispatcher_.SendMessageResponse(sender, MakeError(CommonErrors::success));
  // After sync alert must be sent out -- TO BE IMPLEMENTED
  DoSync(MpidManager::UnresolvedPutMessage(MpidManager::SyncGroupKey(sender),
                                           ActionMpidManagerPutMessage(message),
                                           routing_.kNodeId()));
}

void MpidManagerService::HandleMessageAlert(const nfs_vault::MpidMessageAlert& alert,
                                            const MpidName& receiver) {
  if (!db_.Exists(receiver))
    return;

  DoSync(MpidManager::UnresolvedPutAlert(
      MpidManager::SyncGroupKey(receiver), ActionMpidManagerPutAlert(alert), routing_.kNodeId()));

  if (IsOnline(receiver))
    dispatcher_.SendMessageAlert(alert, receiver);
}

void MpidManagerService::HandleGetMessageRequestFromMpidNode(
    const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver) {
  if (!db_.Exists(alert, receiver))
    return;

  dispatcher_.SendGetMessageRequest(alert, receiver);
}

void MpidManagerService::HandleGetMessageRequest(const nfs_vault::MpidMessageAlert& alert,
                                                 const MpidName& receiver) {
  return dispatcher_.SendGetMessageResponse(db_.GetMessage(alert, receiver), alert.sender,
                                            receiver);
}

void MpidManagerService::HandleGetMessageResponse(
    const nfs_client::MpidMessageOrReturnCode& response, const MpidName& receiver) {
  return dispatcher_.SendGetMessageResponseToMpid(response, receiver);
}

bool MpidManagerService::IsOnline(const MpidName& /*mpid_name*/) {
  return true;
}

void MpidManagerService::HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& receiver) {
  if (!db_.Exists(alert, receiver))
    return;

  DoSync(MpidManager::UnresolvedDeleteAlert(MpidManager::SyncGroupKey(receiver),
                                            ActionMpidManagerDeleteAlert(alert),
                                            routing_.kNodeId()));

  dispatcher_.SendDeleteRequest(alert, receiver);
}

void MpidManagerService::HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& receiver, const MpidName& sender) {
  if (!db_.Exists(alert.sender))
    return;

  auto expected(db_.GetMessage(alert, receiver));
  if (!expected.valid())
    return;

  DoSync(MpidManager::UnresolvedDeleteMessage(MpidManager::SyncGroupKey(sender),
                                              ActionMpidManagerDeleteMessage(alert),
                                              routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
