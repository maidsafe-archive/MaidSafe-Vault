/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/version_handler/service.h"

#include <exception>
#include <string>
#include <utility>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/key.h"
#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/unresolved_action.pb.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/version_handler/action_put.h"
#include "maidsafe/vault/version_handler/version_handler.pb.h"

namespace maidsafe {

namespace vault {

namespace {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() == nfs::Persona::kVersionHandler;
}

template <typename Message>
inline bool FromVersionHandler(const Message& message) {
  return message.destination_persona() == nfs::Persona::kVersionHandler;
}

}  // unnamed namespace


VersionHandlerService::VersionHandlerService(const passport::Pmid& pmid,
                                             routing::Routing& routing,
                                             const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      dispatcher_(routing),
      accumulator_mutex_(),
      close_nodes_change_mutex_(),
      stopped_(false),
      accumulator_(),
      close_nodes_change_(),
      db_(UniqueDbPath(vault_root_dir)),
      kThisNodeId_(routing_.kNodeId()),
      sync_create_version_tree_(NodeId(pmid.name()->string())),
      sync_put_versions_(NodeId(pmid.name()->string())),
      sync_delete_branch_until_fork_(NodeId(pmid.name()->string())),
      account_transfer_() {}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& receiver) {
  typedef nfs::GetVersionsRequestFromMaidNodeToVersionHandler MessageType;
  LOG(kVerbose) << "GetVersionsRequestFromMaidNodeToVersionHandler: " << message.id;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& receiver) {
  typedef nfs::GetBranchRequestFromMaidNodeToVersionHandler MessageType;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& receiver) {
  typedef nfs::GetVersionsRequestFromDataGetterToVersionHandler MessageType;
  LOG(kVerbose) << "GetVersionsRequestFromDataGetterToVersionHandler: " << message.id;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& receiver) {
  typedef nfs::GetBranchRequestFromDataGetterToVersionHandler MessageType;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const PutVersionRequestFromMaidManagerToVersionHandler& message,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Receiver& receiver) {
  typedef PutVersionRequestFromMaidManagerToVersionHandler MessageType;
  LOG(kVerbose) << "PutVersionRequestFromMaidManagerToVersionHandler: " << message.id;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler& message,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Receiver&
       receiver) {
  LOG(kVerbose) << "DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler: " << message.id;
  typedef DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler MessageType;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const CreateVersionTreeRequestFromMaidManagerToVersionHandler& message,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Receiver& receiver) {
  LOG(kVerbose) << "CreateVersionTreeRequestFromMaidManagerToVersionHandler: " << message.id;
  typedef CreateVersionTreeRequestFromMaidManagerToVersionHandler MessageType;
  OperationHandlerWrapper<VersionHandlerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void VersionHandlerService::HandleMessage(
    const SynchroniseFromVersionHandlerToVersionHandler& message,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Sender& sender,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "VersionHandler::HandleMessage SynchroniseFromVersionHandlerToVersionHandler "
                << message.id;
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionVersionHandlerCreateVersionTree::kActionId: {
      VersionHandler::UnresolvedCreateVersionTree unresolved_action(
                                                      proto_sync.serialised_unresolved_action(),
                                                      sender.sender_id, routing_.kNodeId());
      LOG(kVerbose) << "VersionHandlerSync -- CreateVersionTree: " << message.id;
      auto resolved_action(sync_create_version_tree_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        try {
          LOG(kInfo) << "VersionHandlerSync -- CreateVersionTree -Commit: " << message.id;
          db_.Commit(resolved_action->key, resolved_action->action);
          dispatcher_.SendCreateVersionTreeResponse(
              resolved_action->action.originator, resolved_action->key,
              maidsafe_error(CommonErrors::success), resolved_action->action.message_id);
        }
        catch (const maidsafe_error& error) {
          LOG(kError) << message.id << " Failed to create version: "
                      << boost::diagnostic_information(error);
          dispatcher_.SendCreateVersionTreeResponse(
              resolved_action->action.originator, resolved_action->key, error,
                      resolved_action->action.message_id);
        }
      }
      break;
    }
    case ActionVersionHandlerPut::kActionId: {
      VersionHandler::UnresolvedPutVersion unresolved_action(
                                               proto_sync.serialised_unresolved_action(),
                                               sender.sender_id, routing_.kNodeId());
      LOG(kVerbose) << "VersionHandlerSyncPut: " << message.id;
      auto resolved_action(sync_put_versions_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        try {
          LOG(kInfo) << "VersionHandlerSyncPut-Commit: " << message.id;
          db_.Commit(resolved_action->key, resolved_action->action);
          StructuredDataVersions::VersionName tip_of_tree;
          if (resolved_action->action.tip_of_tree) {
            tip_of_tree = *resolved_action->action.tip_of_tree;
          }
          dispatcher_.SendPutVersionResponse(
              resolved_action->action.originator, resolved_action->key, tip_of_tree,
              maidsafe_error(CommonErrors::success), resolved_action->action.message_id);
        }
        catch (const maidsafe_error& error) {
          LOG(kError) << message.id << " Failed to put version: "
                      << boost::diagnostic_information(error);
          dispatcher_.SendPutVersionResponse(
              resolved_action->action.originator, resolved_action->key,
              VersionHandler::VersionName(), error, resolved_action->action.message_id);
        }
      }
      break;
    }
    case ActionVersionHandlerDeleteBranchUntilFork::kActionId: {
      VersionHandler::UnresolvedDeleteBranchUntilFork unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_delete_branch_until_fork_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        try {
          db_.Commit(resolved_action->key, resolved_action->action);
          // BEFORE_RELEASE DOES IT NEED RESPONSE?
        }
        catch (const maidsafe_error& /*error*/) {
          // BEFORE_RELEASE DOES IT NEED REPONSE?
        }
      }
      break;
    }
    default: {
      assert(false);
      LOG(kError) << "Unhandled action type";
    }
  }
}

void VersionHandlerService::HandlePutVersion(
    const VersionHandler::Key& key,
    const VersionHandler::VersionName& old_version,
    const VersionHandler::VersionName& new_version,
    const Identity& originator,
    nfs::MessageId message_id) {
  LOG(kVerbose) << "VersionHandlerService::HandlePutVersion put new version "
                << DebugId(new_version.id) << " after old version " << DebugId(old_version.id)
                << " with message ID : "  << message_id;
  try {
    db_.Get(key);
  }
  catch (const maidsafe_error& error) {
    LOG(kWarning) << "error handling put version request " << message_id;
    dispatcher_.SendPutVersionResponse(originator, key, VersionHandler::VersionName(), error,
                                       message_id);
    return;
  }
  DoSync(VersionHandler::UnresolvedPutVersion(
                      key, ActionVersionHandlerPut(old_version, new_version, originator,
                                                   message_id),
                      routing_.kNodeId()));
}

void VersionHandlerService::HandleDeleteBranchUntilFork(
    const VersionHandler::Key& key, const VersionHandler::VersionName& branch_tip,
    const Identity& /*originator*/) {
  LOG(kVerbose) << "VersionHandlerService::HandleDeleteBranchUntilFork: ";
  DoSync(VersionHandler::UnresolvedDeleteBranchUntilFork(
                      key, ActionVersionHandlerDeleteBranchUntilFork(branch_tip),
                      routing_.kNodeId()));
}

void VersionHandlerService::HandleCreateVersionTree(const VersionHandler::Key& key,
                                                    const VersionHandler::VersionName& version,
                                                    const Identity& originator,
                                                    uint32_t max_versions, uint32_t max_branches,
                                                    nfs::MessageId message_id) {
  LOG(kVerbose) << "VersionHandlerService::HandleCreateVersionTree: " << message_id;
  try {
    db_.Get(key);
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      dispatcher_.SendCreateVersionTreeResponse(originator, key, error, message_id);
      return;
    }
  }
  DoSync(VersionHandler::UnresolvedCreateVersionTree(
                      key, ActionVersionHandlerCreateVersionTree(version, originator, max_versions,
                                                                 max_branches, message_id),
                      routing_.kNodeId()));
}

template <typename UnresolvedAction>
void VersionHandlerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_version_tree_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_put_versions_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_delete_branch_until_fork_,
                                       unresolved_action);
}

// void VersionHandlerService::ValidateClientSender(const nfs::Message& message) const {
//  if (!routing_.IsConnectedClient(message.source().node_id))
//    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::permission_denied));
//  if (!(FromClientMaid(message) || FromClientMpid(message)) || !ForThisPersona(message))
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
// }

// void VersionHandlerService::ValidateSyncSender(const nfs::Message& message) const {
//  if (!routing_.IsConnectedVault(message.source().node_id))
//    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::permission_denied));
//  if (!FromVersionHandler(message) || !ForThisPersona(message))
//    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
// }

// std::vector<StructuredDataVersions::VersionName>
//    VersionHandlerService::GetVersionsFromMessage(const nfs::Message& msg) const {
//   return
// nfs::StructuredData(nfs::StructuredData::serialised_type(msg.data().content)).versions();
// }

// NonEmptyString VersionHandlerService::GetSerialisedRecord(
//    const VersionHandler::DbKey& db_key) {
//  protobuf::UnresolvedEntries proto_unresolved_entries;
//  auto db_value(version_handler_db_.Get(db_key));
//  VersionHandlerKey version_handler_key;
//  //version_handler_key.
//  //VersionHandlerUnresolvedEntry unresolved_entry_db_value(
//  //    std::make_pair(data_name, nfs::MessageAction::kAccountTransfer), metadata_value,
//  //      kThisNodeId_);
//  //auto unresolved_data(sync_.GetUnresolvedData(data_name));
//  //unresolved_data.push_back(unresolved_entry_db_value);
//  //for (const auto& unresolved_entry : unresolved_data) {
//  //  proto_unresolved_entries.add_serialised_unresolved_entry(
//  //      unresolved_entry.Serialise()->string());
//  //}
//  //assert(proto_unresolved_entries.IsInitialized());
//  return NonEmptyString(proto_unresolved_entries.SerializeAsString());
// }

template <>
void VersionHandlerService::HandleMessage(
    const AccountTransferFromVersionHandlerToVersionHandler& message,
    const typename AccountTransferFromVersionHandlerToVersionHandler::Sender& sender,
    const typename AccountTransferFromVersionHandlerToVersionHandler::Receiver& /*receiver*/) {
  LOG(kInfo) << "VersionHandler received account from " << sender.data;
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer";
  }
  for (const auto& serialised_account : account_transfer_proto.serialised_accounts()) {
    HandleAccountTransferEntry(serialised_account, sender);
  }
}

void VersionHandlerService::HandleAccountTransferEntry(
    const std::string& serialised_account, const routing::SingleSource& sender) {
  using Handler = AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kVersionHandler>>;
  protobuf::VersionHandlerKeyValuePair kv_msg;
  if (!kv_msg.ParseFromString(serialised_account)) {
    LOG(kError) << "Failed to parse transferred account";
  }
  auto result(account_transfer_.Add(Key(kv_msg.key()), VersionHandlerValue(kv_msg.value()),
                                    sender.data));
  if (result.result ==  Handler::AddResult::kSuccess) {
    VLOG(nfs::Persona::kVersionHandler, VisualiserAction::kGotAccountTransferred, result.key.name);
    LOG(kVerbose) << "VersionHandlerService got account " << DebugId(result.key.name)
                  << " transferred, having vaule " << result.value->Print();
    AccountType account(std::make_pair(std::move(result.key), std::move(*result.value)));
    HandleAccountTransfer(account);
  } else if (result.result ==  Handler::AddResult::kFailure) {
    LOG(kVerbose) << "VersionHandler AcoccountTransfer SendAccountQuery";
    dispatcher_.SendAccountQuery(result.key);
  }
}

void VersionHandlerService::HandleAccountTransfer(const AccountType& account) {
  try {
    db_.HandleTransfer(std::vector<AccountType> { account });
  }
  catch (const std::exception& error) {
    LOG(kError) << "VersionHandler AcoccountTransfer Failed to store account " << error.what();
    throw;
  }
}

template<>
void VersionHandlerService::HandleMessage(
    const AccountQueryFromVersionHandlerToVersionHandler& /*message*/,
    const typename AccountQueryFromVersionHandlerToVersionHandler::Sender& sender,
    const typename AccountQueryFromVersionHandlerToVersionHandler::Receiver& receiver) {
//  VersionHandler::Key key(MutableData::Name(Identity{ receiver.data.string() }));
  HandleAccountQuery(VersionHandler::Key{ MutableData::Name{ Identity{ receiver.data.string() }}},
                     sender.data);
}

void VersionHandlerService::HandleAccountQuery(const VersionHandler::Key& key,
                                               const NodeId& sender) {
  if (!close_nodes_change_.CheckIsHolder(NodeId(key.name.string()), sender)) {
    LOG(kWarning) << "attempt to obtain account from non-holder";
    return;
  }
  try {
    auto value(db_.Get(key));
    protobuf::AccountTransfer account_transfer_proto;
    protobuf::VersionHandlerKeyValuePair kv_msg;
    kv_msg.set_key(key.Serialise());
    kv_msg.set_value(value.Serialise());
    account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
    dispatcher_.SendAccountQueryResponse(account_transfer_proto.SerializeAsString(),
                                         routing::GroupId(NodeId(key.name.string())), sender);
  }
  catch (const std::exception& error) {
    LOG(kError) << "failed to retrieve account: " << error.what();
  }
}

template <>
void VersionHandlerService::HandleMessage(
    const AccountQueryResponseFromVersionHandlerToVersionHandler& message,
    const typename AccountQueryResponseFromVersionHandlerToVersionHandler::Sender& sender,
    const typename AccountQueryResponseFromVersionHandlerToVersionHandler::Receiver& /*receiver*/) {
  LOG(kInfo) << "VersionHandler received account via response from " << DebugId(sender.sender_id);
  protobuf::AccountTransfer account_transfer_proto;
  if (!account_transfer_proto.ParseFromString(message.contents->data)) {
    LOG(kError) << "Failed to parse account transfer query response";
  }
  assert(account_transfer_proto.serialised_accounts_size() == 1);
  HandleAccountTransferEntry(account_transfer_proto.serialised_accounts(0),
                             routing::SingleSource(sender.sender_id));
}

void VersionHandlerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change_ containing following info before : ";
//   close_nodes_change_.Print();
  std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
  if (stopped_)
    return;
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change containing following info : ";
//   close_nodes_change->Print();
  close_nodes_change_ = *close_nodes_change;

  Db<VersionHandler::Key, VersionHandler::Value>::TransferInfo transfer_info(
      db_.GetTransferInfo(close_nodes_change));
  for (auto& transfer : transfer_info)
    TransferAccount(transfer.first, transfer.second);
//   LOG(kVerbose) << "HandleChurnEvent close_nodes_change_ containing following info after : ";
//   close_nodes_change_.Print();


//  auto record_names(version_handler_db_.GetKeys());
//  auto itr(std::begin(record_names));
//  while (itr != std::end(record_names)) {
//    auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), *itr));
//    auto check_holders_result(CheckHolders(close_nodes_change, routing_.kNodeId(),
//                                           NodeId(result.second)));
//    // Delete records for which this node is no longer responsible.
//    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
//      version_handler_db_.Delete(*itr);
//      itr = record_names.erase(itr);
//      continue;
//    }

//    // Replace old_node(s) in sync object and send TransferRecord to new node(s).
//    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
//    for (auto i(0U); i < check_holders_result.old_holders.size(); ++i) {
//      sync_.ReplaceNode(check_holders_result.old_holders[i], check_holders_result.new_holders[i]);
////      nfs_.TransferRecord(*itr, check_holders_result.new_holders[i],
////                          metadata_handler_.GetSerialisedRecord(record_names));

////      TransferRecord(*itr, check_holders_result.new_holders[i]);
//    }
//    ++itr;
//  }
// TODO(Prakash):  modify ReplaceNodeInSyncList to be called once with vector of tuple/struct
// containing record name, old_holders, new_holders.
}

void VersionHandlerService::TransferAccount(const NodeId& dest,
    const std::vector<Db<VersionHandler::Key, VersionHandler::Value>::KvPair>& accounts) {
  assert(!accounts.empty());
  protobuf::AccountTransfer account_transfer_proto;
  {
    for (auto& account : accounts) {
      VLOG(nfs::Persona::kVersionHandler, VisualiserAction::kAccountTransfer,
           account.first.name, Identity{ dest.string() });
      protobuf::VersionHandlerKeyValuePair kv_pair;
      kv_pair.set_key(account.first.Serialise());
      kv_pair.set_value(account.second.Serialise());
      account_transfer_proto.add_serialised_accounts(kv_pair.SerializeAsString());
      LOG(kVerbose) << "account.first send account " << HexSubstr(account.first.name.string())
                    << " to " << HexSubstr(dest.string())
                    << " with value " << account.second.Print();
    }
  }
  LOG(kVerbose) << "account.firstService::TransferAccount sending account transfer";
  dispatcher_.SendAccountTransfer(dest, account_transfer_proto.SerializeAsString());
}

// void VersionHandlerService::HandleChurnEvent(const NodeId& /*old_node*/,
//                                                    const NodeId& /*new_node*/) {
//    //// for each unresolved entry replace node (only)
//    //{
//    //std::lock_guard<std::mutex> lock(sync_mutex_);
//    //sync_.ReplaceNode(old_node, new_node);
//    //}
//    ////  carry out account transfer for new node !
//    //std::vector<VersionHandler::DbKey> db_keys;
//    //db_keys = version_handler_db_.GetKeys();
//    //for (const auto& key: db_keys) {
//    //  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
//    //  if (routing_.IsNodeIdInGroupRange(NodeId(result.second.string()), new_node) ==
//    //      routing::GroupRangeStatus::kInRange) {  // TODO(dirvine) confirm routing method here
// !!!!!!!!
//    //    // for each db record the new node should have, send it to him (AccountNameFromKey)
//    //  }
//    //}
// }

}  // namespace vault

}  // namespace maidsafe
