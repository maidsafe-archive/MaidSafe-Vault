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

#include "maidsafe/vault/data_manager/service.h"

#include <set>
#include <type_traits>

#include "maidsafe/common/log.h"
#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/utils.h"

#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_add_pmid.h"
#include "maidsafe/vault/data_manager/action_remove_pmid.h"
#include "maidsafe/vault/data_manager/action_node_down.h"
#include "maidsafe/vault/data_manager/action_node_up.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"

namespace maidsafe {

namespace vault {

namespace {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kDataManager;
}

}  // unnamed namespace

DataManagerService::DataManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       nfs_client::DataGetter& data_getter,
                                       const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      asio_service_(2),
      data_getter_(data_getter),
      accumulator_mutex_(),
      close_nodes_change_mutex_(),
      stopped_(false),
      accumulator_(),
      close_nodes_change_(),
      dispatcher_(routing_, pmid),
      get_timer_(asio_service_),
      get_cached_response_timer_(asio_service_),
      db_(UniqueDbPath(vault_root_dir)),
      sync_puts_(NodeId(pmid.name()->string())),
      sync_deletes_(NodeId(pmid.name()->string())),
      sync_add_pmids_(NodeId(pmid.name()->string())),
      sync_remove_pmids_(NodeId(pmid.name()->string())),
      sync_node_downs_(NodeId(pmid.name()->string())),
      sync_node_ups_(NodeId(pmid.name()->string())),
      account_transfer_() {
}

// ==================== Put implementation =========================================================
template <>
void DataManagerService::HandleMessage(
    const PutRequestFromMaidManagerToDataManager& message,
    const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
    const typename PutRequestFromMaidManagerToDataManager::Receiver& receiver) {
  typedef PutRequestFromMaidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void DataManagerService::HandleMessage(
    const PutResponseFromPmidManagerToDataManager& message,
    const typename PutResponseFromPmidManagerToDataManager::Sender& sender,
    const typename PutResponseFromPmidManagerToDataManager::Receiver& receiver) {
  typedef PutResponseFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void DataManagerService::HandleMessage(
    const PutFailureFromPmidManagerToDataManager& message,
    const typename PutFailureFromPmidManagerToDataManager::Sender& sender,
    const typename PutFailureFromPmidManagerToDataManager::Receiver& receiver) {
  typedef PutFailureFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// ==================== Get / IntegrityCheck implementation ========================================
template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromMaidNodeToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodePartialToDataManager& message,
    const typename nfs::GetRequestFromMaidNodePartialToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodePartialToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DataManagerService::HandleMessage GetRequestFromMaidNodePartialToDataManager"
                << " from " << HexSubstr(sender.node_id->string())
                << " for chunk " << HexSubstr(message.contents->raw_name.string())
                << " with message id " << message.id;
  auto data_name(detail::GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromMaidNodePartialToDataManager::SourcePersona SourceType;
  detail::PartialRequestor<SourceType> requestor(sender);
  detail::GetRequestVisitor<DataManagerService, detail::PartialRequestor<SourceType>>
          get_request_visitor(this, requestor, message.id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromDataGetterToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}


// Special case to handle relay messages from partially joined nodes
template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterPartialToDataManager& message,
    const typename nfs::GetRequestFromDataGetterPartialToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterPartialToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromDataGetterPartialToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template<>
void DataManagerService::HandleMessage(
    const GetResponseFromPmidNodeToDataManager& message,
    const typename GetResponseFromPmidNodeToDataManager::Sender& sender,
    const typename GetResponseFromPmidNodeToDataManager::Receiver& receiver) {
  typedef GetResponseFromPmidNodeToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void DataManagerService::HandleMessage(
    const GetCachedResponseFromCacheHandlerToDataManager& message,
    const typename GetCachedResponseFromCacheHandlerToDataManager::Sender& sender,
    const typename GetCachedResponseFromCacheHandlerToDataManager::Receiver& receiver) {
  typedef GetCachedResponseFromCacheHandlerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType &message, const MessageType::Sender &sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

void DataManagerService::HandleGetResponse(const PmidName& pmid_name, nfs::MessageId message_id,
                                           const GetResponseContents& contents) {
  LOG(kVerbose) << "Get content for " << HexSubstr(contents.name.raw_name)
                << " from pmid_name " << HexSubstr(pmid_name.value)
                << " with message_id " << message_id.data;
  try {
    get_timer_.AddResponse(message_id.data, std::make_pair(pmid_name, contents));
  }
  catch (const maidsafe_error& error) {
    // There is scenario that during the procedure of Get, the request side will get timed out
    // earlier than the response side (when they use same time out parameter).
    // So the task will be cleaned out before the time-out response from responder
    // arrived. The policy shall change to keep timer muted instead of throwing.
    // BEFORE_RELEASE handle
    if (error.code() == make_error_code(CommonErrors::no_such_element)) {
      LOG(kInfo) << "DataManagerService::HandleGetResponse task has been removed due to timed out";
    } else {
      LOG(kError) << "DataManagerService::HandleGetResponse encountered unknown error : "
                  << boost::diagnostic_information(error);
      throw;
    }
  }
}

void DataManagerService::HandleGetCachedResponse(nfs::MessageId message_id,
                                                 const GetCachedResponseContents& contents) {
  LOG(kVerbose) << "Get content for " << HexSubstr(contents.name.raw_name)
                << " with message_id " << message_id.data;
  try {
    get_cached_response_timer_.AddResponse(message_id.data, contents);
  }
  catch (...) {
    // BEFORE_RELEASE handle
  }
}

// ==================== Delete implementation ======================================================
template<>
void DataManagerService::HandleMessage(
    const DeleteRequestFromMaidManagerToDataManager& message,
    const typename DeleteRequestFromMaidManagerToDataManager::Sender& sender,
    const typename DeleteRequestFromMaidManagerToDataManager::Receiver& receiver) {
  typedef DeleteRequestFromMaidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

void DataManagerService::SendDeleteRequests(const DataManager::Key& key,
                                            const std::set<PmidName>& pmids,
                                            nfs::MessageId message_id) {
  auto data_name(GetDataNameVariant(key.type, key.name));
  // TODO(Team): DM shall hold the chunk_size info and pass it to PM via delete_request
  int32_t chunk_size(0);
  for (const auto& pmid : pmids) {
    detail::DataManagerSendDeleteVisitor<DataManagerService> delete_visitor(this, chunk_size,
                                                                            pmid, message_id);
    boost::apply_visitor(delete_visitor, data_name);
  }
}

// ==================== Sync / AccountTransfer implementation ======================================
template <>
void DataManagerService::HandleMessage(
    const SynchroniseFromDataManagerToDataManager& message,
    const typename SynchroniseFromDataManagerToDataManager::Sender& sender,
    const typename SynchroniseFromDataManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DataManagerService::HandleMessage SynchroniseFromDataManagerToDataManager";
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data)) {
    LOG(kError) << "SynchroniseFromDataManagerToDataManager can't parse content";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionDataManagerPut::kActionId: {
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerPut";
      DataManager::UnresolvedPut unresolved_action(proto_sync.serialised_unresolved_action(),
                                                   sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit put to db";
        db_.Commit(resolved_action->key, resolved_action->action);
      }
      break;
    }
    case ActionDataManagerDelete::kActionId: {
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete";
      DataManager::UnresolvedDelete unresolved_action(proto_sync.serialised_unresolved_action(),
                                                      sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete "
                   << "resolved for chunk " << HexSubstr(resolved_action->key.name.string());
        auto value(db_.Commit(resolved_action->key, resolved_action->action));
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager ActionDataManagerDelete "
                   << "the chunk " << HexSubstr(resolved_action->key.name.string());
        if (value) {
          assert(value->Subscribers() >= 0);
          if (value->Subscribers() == 0) {
            LOG(kInfo) << "SynchroniseFromDataManagerToDataManager send delete request";
            SendDeleteRequests(resolved_action->key, value->AllPmids(),
                               resolved_action->action.MessageId());
          }
        }
      }
      break;
    }
    case ActionDataManagerAddPmid::kActionId: {
      DataManager::UnresolvedAddPmid unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerAddPmid "
                    << " for chunk " << HexSubstr(unresolved_action.key.name.string())
                    << " and pmid_node " << HexSubstr(unresolved_action.action.kPmidName->string());
      auto resolved_action(sync_add_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit add pmid to db"
                   << " for chunk " << HexSubstr(unresolved_action.key.name.string())
                   << " and pmid_node " << HexSubstr(unresolved_action.action.kPmidName->string());
        try {
          db_.Commit(resolved_action->key, resolved_action->action);
        }
        catch (const maidsafe_error& error) {
          if (error.code() != make_error_code(VaultErrors::account_already_exists))
            throw;
        }
      }
      break;
    }
    case ActionDataManagerRemovePmid::kActionId: {
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerRemovePmid";
      DataManager::UnresolvedRemovePmid unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_remove_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit remove pmid to db";
        // The PmidManager pass down the PutFailure from PmidNode immediately after received it
        // This may cause the sync_remove_pmid got resolved before the sync_add_pmid
        // In that case, the commit will raise an error of no_such_account
        // BEFORE_RELEASE double check whether the "mute" solution is enough
        //                as the pmid_node will get added eventually and may cause problem for get
        try {
          db_.Commit(resolved_action->key, resolved_action->action);
        } catch(maidsafe_error& error) {
          LOG(kWarning) << "having error when trying to commit remove pmid to db : "
                        << boost::diagnostic_information(error);
        }
      }
      break;
    }
    case ActionDataManagerNodeUp::kActionId: {
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerNodeUp";
      DataManager::UnresolvedNodeUp unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_node_ups_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager commit pmid goes online "
                      << " for chunk " << HexSubstr(resolved_action->key.name.string())
                      << " and pmid_node "
                      << HexSubstr(resolved_action->action.kPmidName->string());
        try {
          db_.Commit(resolved_action->key, resolved_action->action);
        } catch(maidsafe_error& error) {
          LOG(kWarning) << "having error when trying to commit set pmid up to db : "
                        << boost::diagnostic_information(error);
        }
      }
      break;
    }
    case ActionDataManagerNodeDown::kActionId: {
      LOG(kVerbose) << "SynchroniseFromDataManagerToDataManager ActionDataManagerNodeDown";
      DataManager::UnresolvedNodeDown unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_node_downs_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromDataManagerToDataManager commit pmid goes offline";
        try {
          db_.Commit(resolved_action->key, resolved_action->action);
        }
        catch (const maidsafe_error& error) {
          if (error.code() != make_error_code(CommonErrors::no_such_element))
            throw;
        }
      }
      break;
    }
    default: {
      LOG(kError) << "SynchroniseFromDataManagerToDataManager Unhandled action type";
      assert(false && "Unhandled action type");
    }
  }
}

template <>
void DataManagerService::HandleMessage(
    const AccountTransferFromDataManagerToDataManager& message,
    const typename AccountTransferFromDataManagerToDataManager::Sender& sender,
    const typename AccountTransferFromDataManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kInfo) << "DataManager received account from " << sender.data;
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer ";
  }
  for (const auto& serialised_account : account_transfer_proto.serialised_accounts()) {
    HandleAccountTransferEntry(serialised_account, sender);
  }
}

template <>
void DataManagerService::HandleMessage(
    const AccountQueryResponseFromDataManagerToDataManager& message,
    const typename AccountQueryResponseFromDataManagerToDataManager::Sender& sender,
    const typename AccountQueryResponseFromDataManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kInfo) << "DataManager received account from " << DebugId(sender.sender_id);
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer ";
  }
  assert(account_transfer_proto.serialised_accounts_size() == 1);
  HandleAccountTransferEntry(account_transfer_proto.serialised_accounts(0),
                             routing::SingleSource(sender.sender_id));
}

void DataManagerService::HandleAccountTransferEntry(
    const std::string& serialised_account, const routing::SingleSource& sender) {
  using Handler = AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kDataManager>>;
  protobuf::DataManagerKeyValuePair kv_msg;
  if (!kv_msg.ParseFromString(serialised_account)) {
    LOG(kError) << "Failed to parse action";
  }
  auto result(account_transfer_.Add(Key(kv_msg.key()), DataManagerValue(kv_msg.value()),
                                    sender.data));
  if (result.result ==  Handler::AddResult::kSuccess) {
    HandleAccountTransfer(std::make_pair(result.key, *result.value));
  } else  if (result.result ==  Handler::AddResult::kFailure) {
    dispatcher_.SendAccountRequest(result.key);
  }
}

void DataManagerService::HandleAccountTransfer(const AccountType& account) {
  try {
    db_.HandleTransfer(std::vector<AccountType> {account});
  }
  catch (const std::exception& error) {
    LOG(kError) << "Failed to store account " << error.what();
    throw;  // MAID-357
  }
}

void DataManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change_ containing following info before : ";
//   close_nodes_change_.Print();
  std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
  if (stopped_)
    return;
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change containing following info : ";
//   close_nodes_change->Print();
  close_nodes_change_ = *close_nodes_change;

  Db<DataManager::Key, DataManager::Value>::TransferInfo transfer_info(
      db_.GetTransferInfo(close_nodes_change));
  for (auto& transfer : transfer_info)
    TransferAccount(transfer.first, transfer.second);
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change_ containing following info after : ";
//   close_nodes_change_.Print();
}

void DataManagerService::TransferAccount(const NodeId& dest,
    const std::vector<Db<DataManager::Key, DataManager::Value>::KvPair>& accounts) {
  // If account just received, shall not pass it out as may under a startup procedure
  // i.e. existing DM will be seen as new_node in close_nodes_change
//  if (account_transfer_.CheckHandled(routing::GroupId(routing_.kNodeId()))) {
//    LOG(kWarning) << "DataManager account just received";
//    return;
//  } MAID-357
  protobuf::AccountTransfer account_transfer_proto;
  for (auto& account : accounts) {
    VLOG(nfs::Persona::kDataManager, VisualiserAction::kAccountTransfer, account.first.name,
         Identity{ dest.string() });
    protobuf::DataManagerKeyValuePair kv_msg;
    kv_msg.set_key(account.first.Serialise());
    kv_msg.set_value(account.second.Serialise());
    account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
    LOG(kVerbose) << "DataManager sent account " << DebugId(account.first.name)
                  << " to " << HexSubstr(dest.string())
                  << " with vaule " << account.second.Print();
  }
  LOG(kVerbose) << "DataManagerService::TransferAccount send account_transfer";
  dispatcher_.SendAccountTransfer(dest, account_transfer_proto.SerializeAsString());
}

template <>
void DataManagerService::HandleMessage(
    const AccountQueryFromDataManagerToDataManager& message,
    const typename AccountQueryFromDataManagerToDataManager::Sender& sender,
    const typename AccountQueryFromDataManagerToDataManager::Receiver& receiver) {
  typedef AccountQueryFromDataManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// ==================== General implementation =====================================================
template <>
void DataManagerService::HandleMessage(
    const SetPmidOnlineFromPmidManagerToDataManager& message,
    const typename SetPmidOnlineFromPmidManagerToDataManager::Sender& sender,
    const typename SetPmidOnlineFromPmidManagerToDataManager::Receiver& receiver) {
  typedef SetPmidOnlineFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void DataManagerService::HandleMessage(
    const SetPmidOfflineFromPmidManagerToDataManager& message,
    const typename SetPmidOfflineFromPmidManagerToDataManager::Sender& sender,
    const typename SetPmidOfflineFromPmidManagerToDataManager::Receiver& receiver) {
  typedef SetPmidOfflineFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

}  // namespace vault

}  // namespace maidsafe
