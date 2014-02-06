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

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/unresolved_action.pb.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/version_handler/action_put.h"
#include "maidsafe/vault/version_handler/key.h"

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
                                             routing::Routing& routing)
    : routing_(routing),
      dispatcher_(routing),
      accumulator_mutex_(),
      accumulator_(),
      db_(),
      kThisNodeId_(routing_.kNodeId()),
      sync_put_versions_(NodeId(pmid.name()->string())),
      sync_delete_branche_until_forks_(NodeId(pmid.name()->string())) {}

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
    const SynchroniseFromVersionHandlerToVersionHandler& message,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Sender& sender,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "VersionHandler::HandleMessage SynchroniseFromVersionHandlerToVersionHandler";
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->data))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionVersionHandlerPut::kActionId: {
      VersionHandler::UnresolvedPutVersion unresolved_action(
                                               proto_sync.serialised_unresolved_action(),
                                               sender.sender_id, routing_.kNodeId());
      LOG(kVerbose) << "VersionHandlerSync: " << message.id;
      auto resolved_action(sync_put_versions_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        try {
          LOG(kInfo) << "VersionHandlerSync-Commit: " << message.id;
          db_.Commit(resolved_action->key, resolved_action->action);
          if (resolved_action->action.tip_of_tree) {
            dispatcher_.SendPutVersionResponse(
                resolved_action->key, *resolved_action->action.tip_of_tree,
                maidsafe_error(CommonErrors::success), resolved_action->action.message_id);
          }
        }
        catch (const maidsafe_error& error) {
          LOG(kError) << message.id << " Failed to put version: " << error.what() ;
          dispatcher_.SendPutVersionResponse(resolved_action->key, VersionHandler::VersionName(),
                                             error, resolved_action->action.message_id);
        }
      }
      break;
    }
    case ActionVersionHandlerDeleteBranchUntilFork::kActionId: {
      VersionHandler::UnresolvedDeleteBranchUntilFork unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_delete_branche_until_forks_.AddUnresolvedAction(unresolved_action));
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
    const VersionHandler::VersionName& new_version, const NodeId& sender,
    nfs::MessageId message_id) {
  LOG(kVerbose) << "VersionHandlerService::HandlePutVersion: " << message_id;
  DoSync(typename VersionHandler::UnresolvedPutVersion(
                      key, ActionVersionHandlerPut(old_version, new_version, sender, message_id),
                      routing_.kNodeId()));
}

void VersionHandlerService::HandleDeleteBranchUntilFork(
    const VersionHandler::Key& key, const VersionHandler::VersionName& branch_tip,
    const NodeId& /*sender*/) {
  LOG(kVerbose) << "VersionHandlerService::HandleDeleteBranchUntilFork: ";
  DoSync(typename VersionHandler::UnresolvedDeleteBranchUntilFork(
                      key, ActionVersionHandlerDeleteBranchUntilFork(branch_tip),
                      routing_.kNodeId()));
}


template <typename UnresolvedAction>
void VersionHandlerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_put_versions_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_delete_branche_until_forks_,
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

void VersionHandlerService::HandleChurnEvent(
    std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {
//  auto record_names(version_handler_db_.GetKeys());
//  auto itr(std::begin(record_names));
//  while (itr != std::end(record_names)) {
//    auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), *itr));
//    auto check_holders_result(CheckHolders(matrix_change, routing_.kNodeId(),
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
  assert(0);
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
