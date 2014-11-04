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

#include "maidsafe/vault/maid_manager/service.h"

#include <map>
#include <string>

#include "boost/thread/future.hpp"

#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/common/data_types/mutable_data.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/maid_manager/value.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"

namespace maidsafe {

namespace vault {

namespace {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMaidManager;
}

}  // unnamed namespace

MaidManagerService::MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       nfs_client::DataGetter& data_getter,
                                       const boost::filesystem::path& /*vault_root_dir*/)
    : routing_(routing),
      data_getter_(data_getter),
      accounts_(),
      accumulator_mutex_(),
      mutex_(),
      nfs_accumulator_(),
      vault_accumulator_(),
      dispatcher_(routing_, pmid),
      sync_create_accounts_(NodeId(pmid.name()->string())),
      sync_remove_accounts_(NodeId(pmid.name()->string())),
      sync_puts_(NodeId(pmid.name()->string())),
      sync_deletes_(NodeId(pmid.name()->string())),
//      account_transfer_(),
      pending_account_mutex_(),
      pending_account_map_() {}

// =============== Maid Account Creation ===========================================================

void MaidManagerService::HandleCreateMaidAccount(const passport::PublicMaid& public_maid,
    const passport::PublicAnmaid& public_anmaid, nfs::MessageId message_id) {
  MaidName account_name(public_maid.name());
  bool exists(false);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it(accounts_.find(account_name));
    exists = (it != std::end(accounts_));
  }

  if (exists) {
    LOG(kError) << "Account for " << account_name.value.string() << " already exists.";
    maidsafe_error error(MakeError(VaultErrors::account_already_exists));
    dispatcher_.SendCreateAccountResponse(account_name, error, message_id);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(pending_account_mutex_);
    MaidAccountCreationStatus account_creation_status(account_name, public_anmaid.name());
    pending_account_map_.insert(std::make_pair(message_id, account_creation_status));
  }

  PmidName pmid_name(Identity(NodeId().string()));
  dispatcher_.SendPutRequest(account_name, public_maid, pmid_name, message_id);
  dispatcher_.SendPutRequest(account_name, public_anmaid, pmid_name, message_id);
}

template <>
void MaidManagerService::HandlePutResponse<passport::PublicMaid>(const MaidName& maid_name,
    const typename passport::PublicMaid::Name& data_name, int64_t /*size*/,
    nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  auto pending_account_itr(pending_account_map_.find(message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    LOG(kInfo) << "Unexpected PublicMaid put response";
    return;
  }
  // In case of a churn, drop it silently
  if (data_name != maid_name)
    return;
  assert(pending_account_itr->second.maid_name == data_name);
  static_cast<void>(data_name);
  pending_account_itr->second.maid_stored = true;

  if (pending_account_itr->second.anmaid_stored) {
    LOG(kVerbose) << "AddLocalAction create account for " << HexSubstr(maid_name->string());
    DoSync(MaidManager::UnresolvedCreateAccount(MaidManager::MetadataKey(maid_name),
        ActionCreateAccount(message_id), routing_.kNodeId()));
  }
}

template <>
void MaidManagerService::HandlePutResponse<passport::PublicAnmaid>(const MaidName& maid_name,
    const typename passport::PublicAnmaid::Name& data_name, int64_t /*size*/,
    nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  auto pending_account_itr(pending_account_map_.find(message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    assert(false);
    return;
  }
  assert(pending_account_itr->second.anmaid_name == data_name);
  static_cast<void>(data_name);
  pending_account_itr->second.anmaid_stored = true;

  if (pending_account_itr->second.maid_stored) {
    LOG(kVerbose) << "AddLocalAction create account for " << HexSubstr(maid_name->string());
    DoSync(MaidManager::UnresolvedCreateAccount(MaidManager::MetadataKey(maid_name),
        ActionCreateAccount(message_id), routing_.kNodeId()));
  }
}

template <>
void MaidManagerService::HandlePutFailure<passport::PublicMaid>(
    const MaidName& maid_name, const passport::PublicMaid::Name& data_name,
    const maidsafe_error& error, nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  auto pending_account_itr(pending_account_map_.find(message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  assert(data_name == maid_name);
  assert(pending_account_itr->second.maid_name == data_name);
  static_cast<void>(data_name);
  // TODO(Team): Consider deleting anmaid key if stored
  pending_account_map_.erase(pending_account_itr);

  dispatcher_.SendCreateAccountResponse(maid_name, error, message_id);
}

template <>
void MaidManagerService::HandlePutFailure<passport::PublicAnmaid>(
    const MaidName& maid_name, const passport::PublicAnmaid::Name& data_name,
    const maidsafe_error& error, nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  auto pending_account_itr(pending_account_map_.find(message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    return;
  }
  assert(pending_account_itr->second.anmaid_name == data_name);
  static_cast<void>(data_name);
  // TODO(Team): Consider deleting anmaid key if stored
  pending_account_map_.erase(pending_account_itr);

  dispatcher_.SendCreateAccountResponse(maid_name, error, message_id);
}

void MaidManagerService::HandleSyncedCreateMaidAccount(
    std::unique_ptr<MaidManager::UnresolvedCreateAccount>&& synced_action) {
  MaidManager::Value value;
  maidsafe_error error(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result(accounts_.insert(std::make_pair(synced_action->key.group_name(), value)));
    if (!result.second)
      error = MakeError(VaultErrors::account_already_exists);
  }
  dispatcher_.SendCreateAccountResponse(synced_action->key.group_name(), error,
                                        synced_action->action.kMessageId);
}

void MaidManagerService::HandleSyncedRemoveMaidAccount(
    std::unique_ptr<MaidManager::UnresolvedRemoveAccount>&& synced_action) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    accounts_.erase(synced_action->key.group_name());
  }
  dispatcher_.SendRemoveAccountResponse(synced_action->key.group_name(),
                                        maidsafe_error(CommonErrors::success),
                                        synced_action->action.kMessageId);
}

// =============== Put/Delete data =================================================================
void MaidManagerService::HandleSyncedPutResponse(
    std::unique_ptr<MaidManager::UnresolvedPut>&& synced_action_put) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(accounts_.find(synced_action_put->key.group_name()));
  if (it == std::end(accounts_))
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  synced_action_put->action(it->second);
}

void MaidManagerService::HandleSyncedDelete(
    std::unique_ptr<MaidManager::UnresolvedDelete>&& /*synced_action_delete*/) {
  //// BEFORE_RELEASE difference process for account_transfer (avoiding double hash)
  // nfs_vault::DataName data_name(synced_action_delete->key.type,
  //                              synced_action_delete->key.name);
  // ObfuscateKey(synced_action_delete->key);
  // try {
  //  auto value(group_db_.Commit(synced_action_delete->key, synced_action_delete->action));
  //  // nullptr will be returned in case of reducing count only
  //  if (value)
  //    dispatcher_.SendDeleteRequest(synced_action_delete->key.group_name(),
  //                                  data_name,
  //                                  synced_action_delete->action.kMessageId);
  //  else
  //    LOG(kInfo) << "DeleteRequest not passed down to DataManager";
  // } catch (const maidsafe_error& error) {
  //  LOG(kError) << "MaidManagerService::HandleSyncedDelete commiting error: "
  //              << boost::diagnostic_information(error);
  //  if (error.code() != make_error_code(CommonErrors::no_such_element) &&
  //      error.code() != make_error_code(VaultErrors::no_such_account)) {
  //    throw;
  //  } else {
  //    // BEFORE_RELEASE trying to delete something not belongs to client shall get muted
  //    VLOG(nfs::Persona::kMaidManager, VisualiserAction::kBlockDeleteRequest, data_name.raw_name);
  //  }
  // }
}

// =============== Account transfer ================================================================

// void MaidManagerService::TransferAccount(const MaidName& account_name,
//                                               const NodeId& new_node) {
//  protobuf::MaidManager maid_account;
//  maid_account.set_maid_name(account_name->string());
//  maid_account.set_serialised_account_details(
//      maid_account_handler_.GetSerialisedAccount(account_name)->string());
//  nfs_.TransferAccount(new_node, NonEmptyString(maid_account.SerializeAsString()));
// }

// void MaidManagerService::HandleAccountTransfer(const nfs::Message& message) {
//  protobuf::MaidManager maid_account;
//  NodeId source_id(message.source().node_id);
//  if (!maid_account.ParseFromString(message.data().content.string()))
//    return;

//  MaidName account_name(Identity(maid_account.maid_name()));
//  bool finished_all_transfers(
//      maid_account_handler_.ApplyAccountTransfer(account_name, source_id,
//        MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details()))));
//  if (finished_all_transfers)
//    UpdatePmidTotals(account_name);
// }

void MaidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  // std::lock_guard<std::mutex> lock(mutex_);
  // if (stopped_)
  //  return;
  // GroupDb<MaidManager>::TransferInfo transfer_info(
  //    group_db_.GetTransferInfo(close_nodes_change));
  // for (auto& transfer : transfer_info)
  //  TransferAccount(transfer.first, transfer.second);
}

void MaidManagerService::TransferAccount(const NodeId& /*dest*/,
    const std::vector<std::pair<MaidName, MaidManagerValue>>& /*accounts*/) {
  //for (auto& account : accounts) {
  //  // If account just received, shall not pass it out as may under a startup procedure
  //  // i.e. existing MM will be seen as new_node in close_nodes_change
  //  if (account_transfer_.CheckHandled(routing::GroupId(NodeId(account.group_name->string())))) {
  //    LOG(kInfo) << "MaidManager account " << HexSubstr(account.group_name->string())
  //               << " just received";
  //    continue;
  //  }
  //  VLOG(nfs::Persona::kMaidManager, VisualiserAction::kAccountTransfer, account.group_name,
  //       Identity{ dest.string() });
  //  try {
  //    std::vector<std::string> actions;
  //    actions.push_back(account.metadata.Serialise());
  //    for (auto& kv : account.kv_pairs) {
  //      protobuf::MaidManagerKeyValuePair kv_msg;
  //        kv_msg.set_key(kv.first.Serialise());
  //        kv_msg.set_value(kv.second.Serialise());
  //        actions.push_back(kv_msg.SerializeAsString());
  //    }
  //    nfs::MessageId message_id(HashStringToMessageId(account.group_name->string()));
  //    MaidManager::UnresolvedAccountTransfer account_transfer(
  //        account.group_name, message_id, actions);
  //    dispatcher_.SendAccountTransfer(dest, account.group_name,
  //                                    message_id, account_transfer.Serialise());
  //    LOG(kVerbose) << "MaidManager sent to " << HexSubstr(dest.string())
  //                  << " with account " << account.Print();
  //  } catch(...) {
  //    // the normal problem is metadata hasn't been populated
  //    LOG(kError) << "MaidManagerService::TransferAccount account info error";
  //  }
  //}
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::PutRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const PutResponseFromDataManagerToMaidManager& message,
    const typename PutResponseFromDataManagerToMaidManager::Sender& sender,
    const typename PutResponseFromDataManagerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef PutResponseFromDataManagerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, VaultAccumulator>(
      vault_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                            return this->ValidateSender(message, sender);
                          },
      VaultAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const PutFailureFromDataManagerToMaidManager& message,
    const typename PutFailureFromDataManagerToMaidManager::Sender& sender,
    const typename PutFailureFromDataManagerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef PutFailureFromDataManagerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, VaultAccumulator>(
      vault_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                            return this->ValidateSender(message, sender);
                          },
      VaultAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMaidNodeToMaidManager& /*message*/,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  // LOG(kVerbose) << message;
  // typedef nfs::DeleteRequestFromMaidNodeToMaidManager MessageType;
  // OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
  //    nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
  //                        return this->ValidateSender(message, sender);
  //                      },
  //    NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
  //    this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::PutVersionRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::CreateAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::RemoveAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const PutVersionResponseFromVersionHandlerToMaidManager& message,
    const typename PutVersionResponseFromVersionHandlerToMaidManager::Sender& sender,
    const typename PutVersionResponseFromVersionHandlerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef PutVersionResponseFromVersionHandlerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, VaultAccumulator>(
      vault_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                            return this->ValidateSender(message, sender);
                          },
      VaultAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const CreateVersionTreeResponseFromVersionHandlerToMaidManager& message,
    const typename CreateVersionTreeResponseFromVersionHandlerToMaidManager::Sender& sender,
    const typename CreateVersionTreeResponseFromVersionHandlerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef CreateVersionTreeResponseFromVersionHandlerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, VaultAccumulator>(
      vault_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                            return this->ValidateSender(message, sender);
                          },
      VaultAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// =============== Sync ============================================================================

// TODO(team): Once all sync messages are implemented, consider specialising HandleSyncedAction for
// each sync action type
template <>
void MaidManagerService::HandleMessage(
    const SynchroniseFromMaidManagerToMaidManager& message,
    const typename SynchroniseFromMaidManagerToMaidManager::Sender& sender,
    const typename SynchroniseFromMaidManagerToMaidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << message;
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data)) {
    LOG(kError) << "SynchroniseFromMaidManagerToMaidManager can't parse the content";
    return;
//     BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionMaidManagerPut::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionMaidManagerPut";
      MaidManager::UnresolvedPut unresolved_action(proto_sync.serialised_unresolved_action(),
                                                   sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedPutResponse";
        HandleSyncedPutResponse(std::move(resolved_action));
      }
      break;
    }
    case ActionMaidManagerDelete::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionMaidManagerDelete";
      MaidManager::UnresolvedDelete unresolved_action(proto_sync.serialised_unresolved_action(),
                                                      sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedDelete";
        HandleSyncedDelete(std::move(resolved_action));
      }
      break;
    }
    case ActionCreateAccount::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionCreateAccount";
      MaidManager::UnresolvedCreateAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_create_accounts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedCreateMaidAccount";
        HandleSyncedCreateMaidAccount(std::move(resolved_action));
      }
      break;
    }
    case ActionRemoveAccount::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionRemoveAccount";
      MaidManager::UnresolvedRemoveAccount unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_remove_accounts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedRemoveMaidAccount";
        HandleSyncedRemoveMaidAccount(std::move(resolved_action));
      }
      break;
    }
    default: {
      LOG(kError) << "Unhandled action type " << proto_sync.action_type();
      assert(false);
    }
  }
}

void MaidManagerService::HandlePutVersionResponse(
    const MaidName& maid_name, const maidsafe_error& return_code,
    std::unique_ptr<StructuredDataVersions::VersionName> tip_of_tree, nfs::MessageId message_id) {
  dispatcher_.SendPutVersionResponse(maid_name, return_code, std::move(tip_of_tree), message_id);
}

void MaidManagerService::HandleCreateVersionTreeResponse(
    const MaidName& maid_name, const maidsafe_error& error , nfs::MessageId message_id) {
    dispatcher_.SendCreateVersionTreeResponse(maid_name, error, message_id);
}

void MaidManagerService::HandleRemoveAccount(const MaidName& maid_name, nfs::MessageId mesage_id) {
  DoSync(MaidManager::UnresolvedRemoveAccount(MaidManager::MetadataKey(maid_name),
      ActionRemoveAccount(mesage_id), routing_.kNodeId()));
}

template <>
void MaidManagerService::HandleMessage(
    const AccountTransferFromMaidManagerToMaidManager& /*message*/,
    const typename AccountTransferFromMaidManagerToMaidManager::Sender& /*sender*/,
    const typename AccountTransferFromMaidManagerToMaidManager::Receiver& /*receiver*/) {
  /*MaidManager::UnresolvedAccountTransfer unresolved_account_transfer(message.contents->data);
  LOG(kInfo) << "MaidManager received account " << DebugId(sender.group_id)
             << " from " << DebugId(sender.sender_id);
  auto resolved_action(account_transfer_.AddUnresolvedAction(
      unresolved_account_transfer, sender,
      AccountTransfer<MaidManager::UnresolvedAccountTransfer>::AddRequestChecker(
          routing::Parameters::group_size / 2)));
  if (resolved_action) {
    LOG(kInfo) << "AccountTransferFromMaidManagerToMaidManager handle account transfer";
    this->HandleAccountTransfer(std::move(resolved_action));
  }*/
}

void MaidManagerService::HandleAccountTransfer(
    std::unique_ptr<MaidManager::UnresolvedAccountTransfer>&& /*resolved_action*/) {
//  VLOG(nfs::Persona::kMaidManager, VisualiserAction::kGotAccountTransferred, resolved_action->key);
//  GroupDb<MaidManager>::Contents content;
//  content.group_name = resolved_action->key;
//  for (auto& action : resolved_action->actions) {
//    try {
//      protobuf::MaidManagerKeyValuePair kv_msg;
//      if (kv_msg.ParseFromString(action)) {
//        LOG(kVerbose) << "HandleAccountTransfer handle key_value pair";
//        MaidManager::Key key(kv_msg.key());
//        LOG(kVerbose) << "HandleAccountTransfer key parsed";
//        MaidManagerValue value(kv_msg.value());
//        LOG(kVerbose) << "HandleAccountTransfer vaule parsed";
//        content.kv_pairs.push_back(std::make_pair(key, std::move(value)));
//      } else {
//        LOG(kVerbose) << "HandleAccountTransfer handle metadata";
//        MaidManagerMetadata meta_data(action);
//        content.metadata = meta_data;
//      }
//    } catch(...) {
//      LOG(kError) << "HandleAccountTransfer can't parse the action";
//    }
//  }
//  LOG(kVerbose) << "MaidManagerService received account "<< content.Print();
//// group_db_.HandleTransfer(content);
}

}  // namespace vault

}  // namespace maidsafe
