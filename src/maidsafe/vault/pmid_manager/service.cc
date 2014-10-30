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
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/utils.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

// PmidName GetPmidAccountName(const nfs::Message& message) {
//  return PmidName(Identity(message.data().name));
// }

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidManager;
}

}  // namespace detail

PmidManagerService::PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       const boost::filesystem::path& vault_root_dir)
    : routing_(routing), db_(UniqueDbPath(vault_root_dir)), accumulator_mutex_(), mutex_(),
      stopped_(false), accumulator_(), dispatcher_(routing_), asio_service_(2),
      get_health_timer_(asio_service_), sync_puts_(NodeId(pmid.name()->string())),
      sync_deletes_(NodeId(pmid.name()->string())),
      sync_set_pmid_health_(NodeId(pmid.name()->string())),
      sync_create_account_(NodeId(pmid.name()->string())),
      account_transfer_() {
}


void PmidManagerService::HandleSyncedPut(
    std::unique_ptr<PmidManager::UnresolvedPut>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedPut commit put for chunk "
                << HexSubstr(synced_action->key.name.string())
                << " to db_ and send_put_response";
  // When different DM choose same PN for the same chunk, PM will receive same Put requests twice
  // from different DM. This will trigger two different sync_put actions and will eventually
  // got two resolved action, and committing same entry to db.
  // BEFORE_RELEASE check how to ensure proper stored_space can be updated
  try {
    PmidManager::MetadataKey metadata_key(synced_action->key.group_name());
    db_.Commit(metadata_key, synced_action->action);
  } catch (const maidsafe_error& error) {
    LOG(kWarning) << "HandleSyncedPut caught an error during db commit " << error.what();
    throw;
  }
  auto data_name(GetDataNameVariant(synced_action->key.type, synced_action->key.name));
  SendPutResponse(data_name, synced_action->key.group_name(),
                  synced_action->action.kSize,
                  synced_action->action.kMessageId);
}


void PmidManagerService::HandleSyncedDelete(
    std::unique_ptr<PmidManager::UnresolvedDelete>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedDelete commit delete for chunk "
                << HexSubstr(synced_action->key.name.string()) << " to db_ ";
  try {
    PmidManager::MetadataKey metadata_key(synced_action->key.group_name());
    db_.Commit(metadata_key, synced_action->action);
  } catch (std::exception& e) {
    // Delete action shall be exception free and no response expected
    LOG(kWarning) << boost::diagnostic_information(e);
  }
}

void PmidManagerService::HandleSyncedSetPmidHealth(
    std::unique_ptr<PmidManager::UnresolvedSetPmidHealth>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedSetAvailableSize for pmid_node "
                << HexSubstr(synced_action->key.name.string()) << " to db_ ";
  // If no account exists, then create an account
  // If has account, and asked to set to 0, delete the account
  // If has account, and asked to set to non-zero, update the size
  PmidManager::MetadataKey metadata_key(synced_action->key);
  db_.Commit(metadata_key, synced_action->action);
}

void PmidManagerService::HandleSyncedCreatePmidAccount(
    std::unique_ptr<PmidManager::UnresolvedCreateAccount>&& synced_action) {
  LOG(kVerbose) << "PmidManagerService::HandleSyncedCreateAccount for pmid_node "
                << HexSubstr(synced_action->key.name.string()) << " to db_ ";
  // If no account exists, then create an account
  // If has account, and asked to set to 0, delete the account
  // If has account, and asked to set to non-zero, update the size
  PmidManager::MetadataKey metadata_key(synced_action->key);
  ActionCreatePmidAccount create_pmid_account_action;
  db_.Commit(metadata_key, create_pmid_account_action);
}

// =============== HandleMessage ===================================================================

template <>
void PmidManagerService::HandleMessage(
    const PutRequestFromDataManagerToPmidManager& message,
    const typename PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "PmidManagerService::HandleMessage PutRequestFromDataManagerToPmidManager"
                << " from " << HexSubstr(sender.sender_id->string())
                << " being asked send to " << HexSubstr(receiver->string());
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
  LOG(kVerbose) << "PmidManagerService::HandleMessage PutFailureFromPmidNodeToPmidManager";
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
    const PmidHealthRequestFromMaidManagerToPmidManager& message,
    const typename PmidHealthRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename PmidHealthRequestFromMaidManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "PmidManagerService:HandleMessage PmidHealthRequestFromMaidManagerToPmidManager";
  typedef PmidHealthRequestFromMaidManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidManagerService::HandleMessage(
    const PmidHealthResponseFromPmidNodeToPmidManager& message,
    const typename PmidHealthResponseFromPmidNodeToPmidManager::Sender& sender,
    const typename PmidHealthResponseFromPmidNodeToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "PmidManagerService::HandleMessage PmidHealthResponseFromPmidNodeToPmidManager";
  typedef PmidHealthResponseFromPmidNodeToPmidManager MessageType;
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
  LOG(kVerbose) << "PmidManagerService::HandleMessage DeleteRequestFromDataManagerToPmidManager";
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
    const GetPmidAccountRequestFromPmidNodeToPmidManager& message,
    const typename GetPmidAccountRequestFromPmidNodeToPmidManager::Sender& sender,
    const typename GetPmidAccountRequestFromPmidNodeToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef GetPmidAccountRequestFromPmidNodeToPmidManager MessageType;
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
  LOG(kVerbose) << message;
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
  LOG(kVerbose) << "PmidManagerService IntegrityCheckRequestFromDataManagerToPmidManager "
                << " received false data notification for chunk "
                << HexSubstr(message.contents->name.raw_name.string())
                << " from " << HexSubstr(sender.sender_id.data.string());
  typedef IntegrityCheckRequestFromDataManagerToPmidManager MessageType;
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
    case ActionPmidManagerSetPmidHealth::kActionId: {
      LOG(kVerbose) << "SynchroniseFromPmidManagerToPmidManager ActionPmidManagerSetAvailableSize";
      PmidManager::UnresolvedSetPmidHealth unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_set_pmid_health_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromPmidManagerToPmidManager HandleSyncedSetAvailableSize";
        HandleSyncedSetPmidHealth(std::move(resolved_action));
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
    default: {
      LOG(kError) << "Unhandled action type";
      assert(false);
    }
  }
}

//=================================================================================================

void PmidManagerService::SendPutResponse(const DataNameVariant& data_name,
                                         const PmidName& pmid_node, int32_t size,
                                         nfs::MessageId message_id) {
  LOG(kInfo) << "PmidManagerService::SendPutResponse";
  detail::PmidManagerPutResponseVisitor<PmidManagerService> put_response(this, size, pmid_node,
                                                                         message_id);
  boost::apply_visitor(put_response, data_name);
}

//=================================================================================================

void PmidManagerService::HandleSendPmidAccount(const PmidName& pmid_node, int64_t available_size) {
  try {
    db_.Get(PmidManager::MetadataKey(pmid_node));
    dispatcher_.SendPmidAccount(pmid_node, nfs_client::ReturnCode(CommonErrors::success));
    DoSync(PmidManager::UnresolvedSetPmidHealth(
        PmidManager::MetadataKey(pmid_node), ActionPmidManagerSetPmidHealth(available_size),
        routing_.kNodeId()));
  } catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
    dispatcher_.SendPmidAccount(pmid_node, nfs_client::ReturnCode(VaultErrors::no_such_account));
  }
}

void PmidManagerService::HandleHealthRequest(const PmidName& pmid_node, const MaidName& maid_node,
                                             nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleHealthRequest from maid_node "
                << HexSubstr(maid_node.value.string()) << " for pmid_node "
                << HexSubstr(pmid_node.value.string()) << " with message_id " << message_id.data;
  auto functor([=](const PmidManagerMetadata& pmid_health) {
    LOG(kVerbose) << "PmidManagerService::HandleHealthRequest "
                  << HexSubstr(pmid_node.value.string())
                  << " task called from timer to DoHandleGetHealthResponse";
    this->DoHandleHealthResponse(pmid_node, maid_node, pmid_health, message_id);
  });
  get_health_timer_.AddTask(detail::Parameters::kDefaultTimeout / 2, functor, 1,
                            message_id.data);
  dispatcher_.SendHealthRequest(pmid_node, message_id);
}

void PmidManagerService::HandleHealthResponse(const PmidName& pmid_name,
                                              uint64_t available_size,
                                              nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleHealthResponse Get pmid_health for "
                << HexSubstr(pmid_name.value) << " with message_id " << message_id.data;
  try {
    PmidManagerMetadata pmid_health;
    pmid_health.SetAvailableSize(available_size);
    get_health_timer_.AddResponse(message_id.data, pmid_health);
  }
  catch (...) {
    // BEFORE_RELEASE handle
  }
}

void PmidManagerService::DoHandleHealthResponse(const PmidName& pmid_node,
    const MaidName& maid_node, const PmidManagerMetadata& pmid_health, nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::DoHandleHealthResponse regarding maid_node "
                << HexSubstr(maid_node.value.string()) << " for pmid_node "
                << HexSubstr(pmid_node.value.string()) << " with message_id " << message_id.data;
  try {
    PmidManagerMetadata reply(pmid_health);
    if (pmid_health == PmidManagerMetadata()) {
      LOG(kInfo) << "PmidManagerService::DoHandleHealthResponse reply with local record";
      reply = db_.Get(PmidManager::MetadataKey(pmid_node));
    } else {
      DoSync(PmidManager::UnresolvedSetPmidHealth(
          PmidManager::MetadataKey(pmid_node),
          ActionPmidManagerSetPmidHealth(pmid_health.claimed_available_size),
          routing_.kNodeId()));
    }
    dispatcher_.SendHealthResponse(maid_node, pmid_node, reply,
                                   message_id, maidsafe_error(CommonErrors::success));
  }
  catch (...) {
    LOG(kInfo) << "PmidManagerService::DoHandleHealthResponse no_such_element";
    dispatcher_.SendHealthResponse(maid_node, pmid_node, PmidManagerMetadata(), message_id,
                                   maidsafe_error(CommonErrors::no_such_element));
  }
}

void PmidManagerService::HandleCreatePmidAccountRequest(const PmidName& pmid_node,
                                                        const MaidName& maid_node,
                                                        nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerService::HandleCreatePmidAccountRequest from maid_node "
                << HexSubstr(maid_node.value.string()) << " for pmid_node "
                << HexSubstr(pmid_node.value.string()) << " with message_id " << message_id.data;
  try {
    db_.Get(PmidManager::MetadataKey(pmid_node));
  } catch(const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "PmidManagerService::HandleCreatePmidAccountRequest vault error : "
                  << boost::diagnostic_information(error);
      throw;
    }
    LOG(kError) << "PmidManagerService::HandleCreatePmidAccountRequest no_such_element";
    // Once synced, check whether account exists or not, if not exist then shall create an account
    // If exist, decide whether to update or delete depending on account status and targeting size
    DoSync(PmidManager::UnresolvedCreateAccount(PmidManager::MetadataKey(pmid_node),
        ActionCreatePmidAccount(), routing_.kNodeId()));
  }
}

// =================================================================================================

// void PmidManagerService::GetPmidTotals(const nfs::Message& message) {
//  try {
//    PmidManagerMetadata
// metadata(pmid_account_handler_.GetMetadata(PmidName(message.data().name)));
//    if (!metadata.pmid_name->string().empty()) {
//      nfs::Reply reply(CommonErrors::success, metadata.Serialise());
//      nfs_.ReturnPmidTotals(message.source().node_id, reply.Serialise());
//    } else {
//      nfs_.ReturnFailure(message);
//    }
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
// }

// void PmidManagerService::GetPmidAccount(const nfs::Message& message) {
//  try {
//    PmidName pmid_name(detail::GetPmidAccountName(message));
//    protobuf::PmidAccountResponse pmid_account_response;
//    protobuf::PmidAccount pmid_account;
//    PmidAccount::serialised_type serialised_account_details;
//    pmid_account.set_pmid_name(pmid_name.data.string());
//    try {
//      serialised_account_details = pmid_account_handler_.GetSerialisedAccount(pmid_name, false);
//      pmid_account.set_serialised_account_details(serialised_account_details.data.string());
//      pmid_account_response.set_status(true);
//    }
//    catch(const maidsafe_error&) {
//      pmid_account_response.set_status(false);
//      pmid_account_handler_.CreateAccount(PmidName(detail::GetPmidAccountName(message)));
//    }
//    pmid_account_response.mutable_pmid_account()->CopyFrom(pmid_account);
//    nfs_.AccountTransfer<passport::Pmid>(
//          pmid_name, NonEmptyString(pmid_account_response.SerializeAsString()));
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
// }

void PmidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_)
      return;
    VLOG(VisualiserAction::kConnectionMap, close_nodes_change->ReportConnection());
/*
 *  PmidManager no longer report the PmidNode status to DataManager to mark node up / down
//     LOG(kVerbose) << "PmidManager HandleChurnEvent processing lost node case";
    auto lost_node(close_nodes_change->lost_node());
    if (!lost_node.IsZero()) {
      LOG(kVerbose) << "PmidManager HandleChurnEvent detected lost_node " << DebugId(lost_node);
      try {
        auto pmid_node(PmidName(Identity(lost_node.string())));
        auto meta_data(db_.Get(PmidManager::MetadataKey(pmid_node)));
        auto contents(db_.GetContents(pmid_node));
        for (const auto& kv_pair : contents.kv_pairs) {
          VLOG(nfs::Persona::kPmidManager, VisualiserAction::kDropPmidNode,
               Identity{ lost_node.string() }, kv_pair.first.name);
          auto data_name(nfs_vault::DataName(kv_pair.first.type, kv_pair.first.name));
          dispatcher_.SendSetPmidOffline(data_name, pmid_node);
          LOG(kVerbose) << "Broadcasting pmid_node " << DebugId(pmid_node) << " holding data "
                        << DebugId(kv_pair.first.name) << " is offline";
        }
      } catch (const maidsafe_error& error) {
        LOG(kVerbose) << "error : " << boost::diagnostic_information(error) << "\n\n";
        if (error.code() != make_error_code(VaultErrors::no_such_account))
          throw;
      }
    }
//     LOG(kVerbose) << "PmidManager HandleChurnEvent processing new_node case";
    auto new_node(close_nodes_change->new_node());
    if (!new_node.IsZero()) {
      LOG(kVerbose) << "PmidManager HandleChurnEvent detected new_node " << DebugId(new_node);
      try {
        auto pmid_node(PmidName(Identity(new_node.string())));
        auto contents(db_.GetContents(PmidName(Identity(new_node.string()))));
        for (const auto& kv_pair : contents.kv_pairs) {
          VLOG(nfs::Persona::kPmidManager, VisualiserAction::kJoinPmidNode,
               Identity{ new_node.string() }, kv_pair.first.name);
          auto data_name(nfs_vault::DataName(kv_pair.first.type, kv_pair.first.name));
          dispatcher_.SendSetPmidOnline(data_name, pmid_node);
        }
      } catch (const maidsafe_error& error) {
//      LOG(kVerbose) << "error : " << boost::diagnostic_information(error) << "\n\n";
        if (error.code() != make_error_code(VaultErrors::no_such_account))
          throw;
      }
    }
*/
//     LOG(kVerbose) << "PmidManager HandleChurnEvent processing account transfer";
    Db<PmidManager::MetadataKey, PmidManager::Metadata>::TransferInfo transfer_info(
        db_.GetTransferInfo(close_nodes_change));
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

void PmidManagerService::TransferAccount(const NodeId& dest,
    const std::vector<Db<PmidManager::MetadataKey, PmidManager::Metadata>::KvPair>& accounts) {
  for (auto& account : accounts) {
    // If account just received, shall not pass it out as may under a startup procedure
    // i.e. existing PM will be seen as new_node in close_nodes_change
    if (account_transfer_.CheckHandled(
        routing::GroupId(NodeId(account.first.name.string())))) {
      LOG(kInfo) << "PmidManager account " << HexSubstr(account.first.name.string())
                 << " just received";
      continue;
    }
    VLOG(nfs::Persona::kPmidManager, VisualiserAction::kAccountTransfer,
         account.first.name, Identity{ dest.string() });
    try {
      std::vector<std::string> actions;
      actions.push_back(account.second.Serialise());
      LOG(kVerbose) << "PmidManagerService::TransferAccount metadata serialised";
      nfs::MessageId message_id(HashStringToMessageId(account.first.name.string()));
      PmidName pmid_name(Identity(account.first.name.string()));
      PmidManager::UnresolvedAccountTransfer account_transfer(pmid_name, message_id, actions);
      dispatcher_.SendAccountTransfer(dest, PmidName(account.first.name),
                                      message_id, account_transfer.Serialise());
#ifdef TESTING
      LOG(kVerbose) << "PmidManager sent to " << HexSubstr(dest.string())
                    << " with account " << account.second.Print();
#endif
    } catch(...) {
      // normally, the problem is metadata hasn't populated
      LOG(kError) << "PmidManagerService::TransferAccount account info error";
    }
  }
}

template <>
void PmidManagerService::HandleMessage(
    const AccountTransferFromPmidManagerToPmidManager& message,
    const typename AccountTransferFromPmidManagerToPmidManager::Sender& sender,
    const typename AccountTransferFromPmidManagerToPmidManager::Receiver& /*receiver*/) {
  PmidManager::UnresolvedAccountTransfer unresolved_account_transfer(message.contents->data);
  LOG(kInfo) << "PmidManager received account " << DebugId(sender.group_id)
             << " from " << DebugId(sender.sender_id);
  auto resolved_action(account_transfer_.AddUnresolvedAction(
      unresolved_account_transfer, sender,
      AccountTransfer<PmidManager::UnresolvedAccountTransfer>::AddRequestChecker(
          routing::Parameters::group_size / 2)));
  if (resolved_action) {
    LOG(kInfo) << "AccountTransferFromPmidManagerToPmidManager handle account transfer";
    this->HandleAccountTransfer(std::move(resolved_action));
  }
}

void PmidManagerService::HandleAccountTransfer(
    std::unique_ptr<PmidManager::UnresolvedAccountTransfer>&& resolved_action) {
  VLOG(nfs::Persona::kPmidManager, VisualiserAction::kGotAccountTransferred,
       resolved_action->key);
  std::vector<std::pair<PmidManager::MetadataKey, PmidManagerMetadata>> kv_pairs;
  for (auto& action : resolved_action->actions) {
    try {
      LOG(kVerbose) << "HandleAccountTransfer handle metadata";
      PmidManagerMetadata meta_data(action);
      PmidManager::MetadataKey meta_data_key(resolved_action->key);
      kv_pairs.push_back(std::make_pair(std::move(meta_data_key), std::move(meta_data)));
    } catch(...) {
      LOG(kError) << "HandleAccountTransfer can't parse the action";
    }
  }
  db_.HandleTransfer(kv_pairs);
}

// void PmidManagerService::TransferAccount(const PmidName& account_name, const NodeId& new_node) {
//  protobuf::PmidAccount pmid_account;
//  pmid_account.set_pmid_name(account_name.data.string());
//  PmidAccount::serialised_type
//    serialised_account_details(pmid_account_handler_.GetSerialisedAccount(account_name, true));
//  pmid_account.set_serialised_account_details(serialised_account_details.data.string());
//  nfs_.TransferAccount(new_node, NonEmptyString(pmid_account.SerializeAsString()));
// }

// void PmidManagerService::HandleAccountTransfer(const nfs::Message& message) {
//  protobuf::PmidAccount pmid_account;
//  NodeId source_id(message.source().node_id);
//  if (!pmid_account.ParseFromString(message.data().content.string()))
//    return;

//  PmidName account_name(Identity(pmid_account.pmid_name()));
//  bool finished_all_transfers(
//      pmid_account_handler_.ApplyAccountTransfer(account_name, source_id,
//        PmidAccount::serialised_type(NonEmptyString(pmid_account.serialised_account_details()))));
//  if (finished_all_transfers)
//    return;    // TODO(Team) Implement whatever else is required here?
// }

}  // namespace vault

}  // namespace maidsafe
