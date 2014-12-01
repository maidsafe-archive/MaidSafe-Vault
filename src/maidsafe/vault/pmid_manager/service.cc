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

#include "maidsafe/vault/pmid_manager/service.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/utils.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidManager;
}

}  // namespace detail

PmidManagerService::PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing)
    : routing_(routing), accounts_(), accumulator_mutex_(), mutex_(),
      stopped_(false), accumulator_(), dispatcher_(routing_), asio_service_(2),
      get_health_timer_(asio_service_), sync_puts_(NodeId(pmid.name()->string())),
      sync_deletes_(NodeId(pmid.name()->string())),
      sync_create_account_(NodeId(pmid.name()->string())),
      sync_update_account_(NodeId(pmid.name()->string())),
      account_transfer_() {
}

void PmidManagerService::HandleSyncedPut(
    std::unique_ptr<PmidManager::UnresolvedPut>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedPut commit put for chunk "
                << HexSubstr(synced_action->key.name.string())
                << " to accounts and send_put_response";
  // When different DM choose same PN for the same chunk, PM will receive same Put requests twice
  // from different DM. This will trigger two different sync_put actions and will eventually
  // got two resolved action, and committing same entry to accounts_
  // BEFORE_RELEASE check how to ensure proper stored_space can be updated
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    PmidManager::Key account_name(synced_action->key.group_name());
    auto itr(accounts_.find(account_name));
    if (itr == std::end(accounts_)) {
      // create an empty account for non-registered pmid_node
      auto result(accounts_.insert(std::make_pair(account_name, PmidManager::Value())));
      if (result.second)
        itr = result.first;
      else
        BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_error));
      synced_action->action(itr->second);
    }
    synced_action->action(itr->second);
  } catch (const maidsafe_error& error) {
    LOG(kWarning) << "HandleSyncedPut caught an error during account commit " << error.what();
    throw;
  }
  auto data_name(GetDataNameVariant(synced_action->key.type, synced_action->key.name));
  SendPutResponse(data_name, synced_action->key.group_name(),
                  synced_action->action.kMessageId);
}

void PmidManagerService::HandleSyncedDelete(
    std::unique_ptr<PmidManager::UnresolvedDelete>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedDelete commit delete for chunk "
                << HexSubstr(synced_action->key.name.string()) << " to accounts_ ";
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    PmidManager::Key account_name(synced_action->key.group_name());
    accounts_.erase(account_name);
  } catch (std::exception& e) {
    // Delete action shall be exception free and no response expected
    LOG(kWarning) << boost::diagnostic_information(e);
  }
}

void PmidManagerService::HandleSyncedCreatePmidAccount(
    std::unique_ptr<PmidManager::UnresolvedCreateAccount>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedCreateAccount for pmid_node "
                << HexSubstr(synced_action->key.group_name()->string()) << " to accounts_ ";
  // If no account exists, then create an account. otherwise ignore the request
  std::lock_guard<std::mutex> lock(mutex_);
  PmidManager::Key account_name(synced_action->key.group_name());
  auto itr(accounts_.find(account_name));
  if (itr == std::end(accounts_))
    accounts_.insert(std::make_pair(account_name, PmidManager::Value()));
}

void PmidManagerService::HandleSyncedUpdateAccount(
    std::unique_ptr<PmidManager::UnresolvedUpdateAccount>&& synced_action) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    PmidManager::Key account_name(synced_action->key.group_name());
    auto itr(accounts_.find(account_name));
    if (itr == std::end(accounts_))
      LOG(kWarning) << "accound does not exist";
    synced_action->action(itr->second);
  } catch (const maidsafe_error& error) {
    LOG(kWarning) << "HandleSyncedUpdateAccount error " << error.what();
    throw;
  }
}

// =============== HandleMessage ===================================================================

template <>
void PmidManagerService::HandleMessage(
    const PutRequestFromDataManagerToPmidManager& message,
    const typename PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef PutRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidManagerService::HandleMessage(
    const PutFailureFromPmidNodeToPmidManager& message,
    const typename PutFailureFromPmidNodeToPmidManager::Sender& sender,
    const typename PutFailureFromPmidNodeToPmidManager::Receiver& receiver) {
  typedef PutFailureFromPmidNodeToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidManagerService::HandleMessage(
    const DeleteRequestFromDataManagerToPmidManager& message,
    const typename DeleteRequestFromDataManagerToPmidManager::Sender& sender,
    const typename DeleteRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef DeleteRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidManagerService::HandleMessage(
    const CreatePmidAccountRequestFromMaidManagerToPmidManager& message,
    const typename CreatePmidAccountRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename CreatePmidAccountRequestFromMaidManagerToPmidManager::Receiver& receiver) {
  typedef CreatePmidAccountRequestFromMaidManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidManagerService::HandleMessage(
    const IntegrityCheckRequestFromDataManagerToPmidManager& message,
    const typename IntegrityCheckRequestFromDataManagerToPmidManager::Sender& sender,
    const typename IntegrityCheckRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef IntegrityCheckRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template<>
void PmidManagerService::HandleMessage(
    const UpdateAccountFromDataManagerToPmidManager& message,
    const typename UpdateAccountFromDataManagerToPmidManager::Sender& sender,
    const typename UpdateAccountFromDataManagerToPmidManager::Receiver& receiver) {
  using  MessageType = UpdateAccountFromDataManagerToPmidManager;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// =============== Handle Sync Messages ============================================================

template<>
void PmidManagerService::HandleMessage(
    const SynchroniseFromPmidManagerToPmidManager& message,
    const typename SynchroniseFromPmidManagerToPmidManager::Sender& sender,
    const typename SynchroniseFromPmidManagerToPmidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << message;
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data)) {
    LOG(kError) << "SynchroniseFromPmidManagerToPmidManager can't parse content";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionPmidManagerPut::kActionId: {
      LOG(kVerbose) << "SynchroniseFromPmidManagerToPmidManager ActionPmidManagerPut";
      PmidManager::UnresolvedPut unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromPmidManagerToPmidManager HandleSyncedPut";
        HandleSyncedPut(std::move(resolved_action));
      }
      break;
    }
    case ActionPmidManagerDelete::kActionId: {
      LOG(kVerbose) << "SynchroniseFromPmidManagerToPmidManager ActionPmidManagerDelete";
      PmidManager::UnresolvedDelete unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromPmidManagerToPmidManager HandleSyncedDelete";
        HandleSyncedDelete(std::move(resolved_action));
      }
      break;
    }
    case ActionCreatePmidAccount::kActionId: {
      LOG(kVerbose) << "SynchroniseFromPmidManagerToPmidManager ActionCreatePmidAccount";
      PmidManager::UnresolvedCreateAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_create_account_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromPmidManagerToPmidManager HandleSyncedCreateAccount";
        HandleSyncedCreatePmidAccount(std::move(resolved_action));
      }
      break;
    }
    case ActionPmidManagerUpdateAccount::kActionId: {
      LOG(kVerbose) << "SynchroniseFromPmidManagerToPmidManager ActionPmidManagerUpdateAccount";
      PmidManager::UnresolvedUpdateAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_update_account_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromPmidManagerToPmidManager HandleSyncedUpdateAccount";
        HandleSyncedUpdateAccount(std::move(resolved_action));
      }
      break;
    }
    default: {
      LOG(kError) << "Unhandled action type";
      assert(false);
    }
  }
}

//=================================================================================================

void PmidManagerService::SendPutResponse(const DataNameVariant& data_name,
                                         const PmidName& pmid_node, nfs::MessageId message_id) {
  LOG(kInfo) << "PmidManagerService::SendPutResponse";
  detail::PmidManagerPutResponseVisitor<PmidManagerService> put_response(this, pmid_node,
                                                                         message_id);
  boost::apply_visitor(put_response, data_name);
}

//=================================================================================================

void PmidManagerService::HandleCreatePmidAccountRequest(const PmidName& pmid_node,
                                                        const MaidName& maid_node,
                                                        nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleCreatePmidAccountRequest from maid_node "
                << HexSubstr(maid_node.value.string()) << " for pmid_node "
                << HexSubstr(pmid_node.value.string()) << " with message_id " << message_id.data;
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(accounts_.find(PmidManager::Key(pmid_node)));
  if (itr != std::end(accounts_))
    DoSync(PmidManager::UnresolvedCreateAccount(PmidManager::SyncGroupKey(pmid_node),
        ActionCreatePmidAccount(), routing_.kNodeId()));
  else
    LOG(kError) << "PmidManagerService::HandleCreatePmidAccountRequest account already existed";
}

// =================================================================================================

void PmidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_)
      return;
    VLOG(VisualiserAction::kConnectionMap, close_nodes_change->ReportConnection());

//     LOG(kVerbose) << "PmidManager HandleChurnEvent processing account transfer";
    const auto transfer_info(
        detail::GetTransferInfo<PmidManager::Key, PmidManager::Value, PmidManager::TransferInfo>(
            close_nodes_change, accounts_));
    for (auto& transfer : transfer_info)
      TransferAccount(transfer.first, transfer.second);
//     LOG(kVerbose) << "PmidManager HandleChurnEvent completed";
  } catch (const std::exception& e) {
    LOG(kVerbose) << "error : " << boost::diagnostic_information(e) << "\n\n";
  }
}

// void PmidManagerService::ValidateDataSender(const nfs::Message& message) const {
//  if (!message.HasDataHolder()
//      || !routing_.IsConnectedVault(NodeId(message.pmid_node()->string()))
//      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
//    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::permission_denied));

//  if (!FromDataManager(message) || !detail::ForThisPersona(message))
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
// }

// void PmidManagerService::ValidateGenericSender(const nfs::Message& message) const {
//  if (!routing_.IsConnectedVault(message.source().node_id)
//      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
//    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::permission_denied));

//  if (!FromDataManager(message) || !FromPmidNode(message) || !detail::ForThisPersona(message))
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
// }

// =============== Account transfer ===============================================================

void PmidManagerService::TransferAccount(const NodeId& peer,
                                         const std::vector<PmidManager::KvPair>& accounts) {
  assert(!accounts.empty());
  protobuf::AccountTransfer account_transfer_proto;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& account : accounts) {
      VLOG(nfs::Persona::kPmidManager, VisualiserAction::kAccountTransfer,
           account.first, Identity{ peer.string() });
      protobuf::PmidManagerKeyValuePair kv_pair;
      MetadataKey<PmidName> key(account.first);
      kv_pair.set_key(key.Serialise());
      kv_pair.set_value(account.second.Serialise());
      account_transfer_proto.add_serialised_accounts(kv_pair.SerializeAsString());
      LOG(kVerbose) << "PmidManager send account " << HexSubstr(account.first->string())
                    << " to " << HexSubstr(peer.string())
                    << " with value " << account.second.Print();
    }
  }
  LOG(kVerbose) << "PmidManagerService::TransferAccount send account transfer";
  dispatcher_.SendAccountTransfer(peer, account_transfer_proto.SerializeAsString());
}

template <>
void PmidManagerService::HandleMessage(
    const AccountTransferFromPmidManagerToPmidManager& message,
    const typename AccountTransferFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountTransferFromPmidManagerToPmidManager::Receiver& /*receiver*/) {
  LOG(kInfo) << "PmidManager received account from " << sender.data;
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer ";
  }
  for (const auto& serialised_account : account_transfer_proto.serialised_accounts())
    HandleAccountTransferEntry(serialised_account, sender);
}

void PmidManagerService::HandleAccountTransfer(const AccountType& account) {
//  VLOG(nfs::Persona::kPmidManager, VisualiserAction::kGotAccountTransferred,
//       account.first);
  std::lock_guard<std::mutex> lock(mutex_);
  accounts_.insert(account);
}

template<>
void PmidManagerService::HandleMessage(
    const AccountQueryFromPmidManagerToPmidManager& /*message*/,
    const typename AccountQueryFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountQueryFromPmidManagerToPmidManager::Receiver& receiver) {
  HandleAccountQuery(PmidManager::Key{ Identity{ receiver.data.string() }} , sender.data);
}

template<>
void PmidManagerService::HandleMessage(
    const AccountQueryResponseFromPmidManagerToPmidManager& message,
    const typename AccountQueryResponseFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountQueryResponseFromPmidManagerToPmidManager::Receiver& /*receiver*/) {
  LOG(kInfo) << "PmidManager received account from " << sender.sender_id;
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer ";
  }
  assert(account_transfer_proto.serialised_accounts_size() == 1);
  HandleAccountTransferEntry(account_transfer_proto.serialised_accounts(0),
                             routing::SingleSource(sender.sender_id));
}

void PmidManagerService::HandleAccountTransferEntry(
    const std::string& serialised_account, const routing::SingleSource& sender) {
  using Handler = AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kPmidManager>>;
  protobuf::PmidManagerKeyValuePair kv_msg;
  if (!kv_msg.ParseFromString(serialised_account)) {
    LOG(kError) << "Failed to parse action";
  }

  auto result(account_transfer_.Add(
      PmidManager::Key(MetadataKey<PmidName>(kv_msg.key()).group_name()),
      PmidManagerValue(kv_msg.value()), sender.data));
  if (result.result ==  Handler::AddResult::kSuccess) {
    HandleAccountTransfer(std::make_pair(result.key, *result.value));
  } else  if (result.result ==  Handler::AddResult::kFailure) {
    dispatcher_.SendAccountQuery(result.key);
  }
}

void PmidManagerService::HandleAccountQuery(const PmidManager::Key& key, const NodeId& sender) {
//  if (!close_nodes_change_.CheckIsHolder(NodeId(name->string()), sender)) {
//    LOG(kWarning) << "attempt to obtain account for non-holder";
//    return;
//  }  MAID-360 investigate the check is required(mmoadeli)
  try {
    auto value(accounts_.at(key));
    protobuf::AccountTransfer account_transfer_proto;
    protobuf::PmidManagerKeyValuePair kv_msg;
    kv_msg.set_key(MetadataKey<PmidManager::Key>(key).Serialise());
    kv_msg.set_value(value.Serialise());
    account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
    dispatcher_.SendAccountQueryResponse(account_transfer_proto.SerializeAsString(),
                                        routing::GroupId(NodeId(key->string())), sender);
  }
  catch (const std::exception& error) {
    LOG(kError) << "failed to retrieve account: " << error.what();
  }
}

void PmidManagerService::HandleUpdateAccount(const PmidName& pmid_node, int32_t diff_size) {
  DoSync(PmidManager::UnresolvedUpdateAccount(PmidManager::SyncGroupKey(pmid_node),
      ActionPmidManagerUpdateAccount(diff_size), routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
