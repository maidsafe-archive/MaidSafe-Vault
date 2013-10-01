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

#include <string>
#include <vector>

#include "maidsafe/routing/parameters.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/utils.h"

#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_add_pmid.h"
#include "maidsafe/vault/data_manager/action_remove_pmid.h"
#include "maidsafe/vault/data_manager/action_node_down.h"
#include "maidsafe/vault/data_manager/action_node_up.h"
#include "maidsafe/vault/operations_visitor.h"

namespace maidsafe {

namespace vault {

namespace {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kDataManager;
}

}  // unnamed namespace

DataManagerService::DataManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       nfs_client::DataGetter& data_getter)
    : routing_(routing),
      asio_service_(2),
      data_getter_(data_getter),
      accumulator_mutex_(),
      accumulator_(),
      dispatcher_(routing_, pmid),
      integrity_check_timer_(asio_service_),
      get_timer_(asio_service_),
      db_(),
      sync_puts_(),
      sync_deletes_(),
      sync_add_pmids_(),
      sync_remove_pmids_(),
      sync_node_downs_(),
      sync_node_ups_() {}

// PutRequestFromMaidManagerToDataManager
template <>
void DataManagerService::HandleMessage(
    const PutRequestFromMaidManagerToDataManager& message,
    const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
    const typename PutRequestFromMaidManagerToDataManager::Receiver& receiver) {
  typedef PutRequestFromMaidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// PutResponseFromPmidManagerToDataManager
template <>
void DataManagerService::HandleMessage(
    const PutFailureFromPmidManagerToDataManager& message,
    const typename PutFailureFromPmidManagerToDataManager::Sender& sender,
    const typename PutFailureFromPmidManagerToDataManager::Receiver& receiver) {
  typedef PutFailureFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// GetRequestFromMaidNodeToDataManager
template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromMaidNodeToDataManager MessageType;
  detail::GetRequestVisitor<DataManagerService, typename MessageType::Sender> visitor(
      this, sender, message.message_id);

  PublicMessages public_variant_message;
  if (!nfs::GetVariant(message, public_variant_message))
    return;

  boost::apply_visitor(demuxer, public_variant_message);
  return true;

  boost::apply_visitor(visitor, public_variant_message);

}

// GetRequestFromPmidNodeToDataManager
template<>
void DataManagerService::HandleMessage(
    const GetRequestFromPmidNodeToDataManager& message,
    const typename GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename GetRequestFromPmidNodeToDataManager::Receiver& receiver) {
  typedef GetRequestFromPmidNodeToDataManager MessageType;
}

// GetRequestFromDataGetterToDataManager
template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromDataGetterToDataManager MessageType;
}

// GetResponseFromPmidNodeToDataManager
template<>
void DataManagerService::HandleMessage(
    const GetResponseFromPmidNodeToDataManager& message,
    const typename GetResponseFromPmidNodeToDataManager::Sender& sender,
    const typename GetResponseFromPmidNodeToDataManager::Receiver& receiver) {
  typedef GetResponseFromPmidNodeToDataManager MessageType;
}

template<>
void DataManagerService::HandleMessage(
    const DeleteRequestFromMaidManagerToDataManager& message,
    const typename DeleteRequestFromMaidManagerToDataManager::Sender& sender,
    const typename DeleteRequestFromMaidManagerToDataManager::Receiver& receiver) {
  typedef DeleteRequestFromMaidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

// =============== Sync ============================================================================
template <>
void DataManagerService::HandleMessage(
    const SynchroniseFromDataManagerToDataManager& message,
    const typename SynchroniseFromDataManagerToDataManager::Sender& sender,
    const typename SynchroniseFromDataManagerToDataManager::Receiver& /*receiver*/) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->content.string()))
    ThrowError(CommonErrors::parsing_error);

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionDataManagerPut::kActionId: {
      DataManager::UnresolvedPut unresolved_action(proto_sync.serialised_unresolved_action(),
                                                   sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        db_.Commit(resolved_action->key, resolved_action->action);
      }
      break;
    }
    case ActionDataManagerDelete::kActionId: {
      DataManager::UnresolvedDelete unresolved_action(proto_sync.serialised_unresolved_action(),
                                                      sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        auto value(db_.Commit(resolved_action->key, resolved_action->action));
        if (value->Subscribers() == 0) {
          SendDeleteRequests(resolved_action->key, value->Pmids(), message.message_id);
        }
      }
      break;
    }

    //    case ActionDataManagerDelete::kActionId: {
    //      DataManager::UnresolvedDelete unresolved_action(
    //          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
    //      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
    //      if (resolved_action)
    //        db_.Commit(resolved_action->key, resolved_action->action);
    //      break;
    //    }
    //    case ActionDataManagerAddPmid::kActionId: {
    //      DataManager::UnresolvedAddPmid unresolved_action(
    //          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
    //      auto resolved_action(sync_add_pmids_.AddUnresolvedAction(unresolved_action));
    //      if (resolved_action)
    //        db_.Commit(resolved_action->key, resolved_action->action);
    //      break;
    //    }
    //    case ActionDataManagerRemovePmid::kActionId: {
    //      DataManager::UnresolvedRemovePmid unresolved_action(
    //          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
    //      auto resolved_action(sync_remove_pmids_.AddUnresolvedAction(unresolved_action));
    //      if (resolved_action)
    //        db_.Commit(resolved_action->key, resolved_action->action);
    //      break;
    //    }
    //    case ActionDataManagerNodeUp::kActionId: {
    //      DataManager::UnresolvedNodeUp unresolved_action(
    //          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
    //      auto resolved_action(sync_node_ups_.AddUnresolvedAction(unresolved_action));
    //      if (resolved_action)
    //        db_.Commit(resolved_action->key, resolved_action->action);
    //      break;
    //    }
    //    case ActionDataManagerNodeDown::kActionId: {
    //      DataManager::UnresolvedNodeDown unresolved_action(
    //          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
    //      auto resolved_action(sync_node_downs_.AddUnresolvedAction(unresolved_action));
    //      if (resolved_action)
    //        db_.Commit(resolved_action->key, resolved_action->action);
    //      break;
    //    }
    default: {
      assert(false);
      LOG(kError) << "Unhandled action type";
    }
  }
}

void DataManagerService::SendDeleteRequests(const DataManager::Key& key,
                                            const std::set<PmidName>& pmids,
                                            const nfs::MessageId& message_id) {
  auto data_name(GetDataNameVariant(key.type, key.name));
  for (auto pmid : pmids) {
    detail::DataManagerSendDeleteVisitor<DataManagerService> delete_visitor(this, pmid, message_id);
    boost::apply_visitor(delete_visitor, data_name);
  }
}

void DataManagerService::DoSync() {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_add_pmids_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_pmids_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_node_downs_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_node_ups_);
}

void DataManagerService::HandleDataIntergirity(const IntegrityCheckResponse& response,
                                               const nfs::MessageId& message_id) {
  try {
    integrity_check_timer_.AddResponse(message_id.data, response);
  }
  catch (const std::exception /*ex*/) {
    // Failure to find the task
  }
}

// =============== Churn ===========================================================================
// void DataManagerService::HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change) {
//  auto record_names(metadata_handler_.GetRecordNames());
//  auto itr(std::begin(record_names));
//  auto name(itr->name());
//  while (itr != std::end(record_names)) {
//    auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), name));
//    auto check_holders_result(matrix_change->(NodeId(result.second)));
//    // Delete records for which this node is no longer responsible.
//    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
//      metadata_handler_.DeleteRecord(itr->name());
//      itr = record_names.erase(itr);
//      continue;
//    }

//    // Replace old_node(s) in sync object and send TransferRecord to new node(s).
//    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
//    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
//      metadata_handler_.ReplaceNodeInSyncList(itr->name(), check_holders_result.old_holders[i],
//                                              check_holders_result.new_holders[i]);
//      TransferRecord(itr->name(), check_holders_result.new_holders[i]);
//    }
//    ++itr;
//  }
//  // TODO(Prakash):  modify ReplaceNodeInSyncList to be called once with vector of tuple/struct
//  // containing record name, old_holders, new_holders.
//}

}  // namespace vault

}  // namespace maidsafe
