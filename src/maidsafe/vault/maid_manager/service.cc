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

#include <string>

#include "maidsafe/nfs/vault/pmid_registration.h"
#include "maidsafe/data_types/owner_directory.h"
#include "maidsafe/data_types/group_directory.h"
#include "maidsafe/data_types/world_directory.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/helpers.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"


namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
int32_t EstimateCost(const T&) {
  return 0;
}

template<>
int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&) {
  return 0;
}

template<>
int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&) {
  return 0;
}

template<>
int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&) {
  return 0;
}

//MaidName GetMaidAccountName(const nfs::Message& message) {
//  return MaidName(Identity(message.source().node_id.string()));
//}

}  // namespace detail

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMaidManager;
}

template<typename T>
T Merge(std::vector<T> values) {
  std::map<T, size_t> value_and_count;
  auto most_frequent_itr(std::end(values));
  size_t most_frequent(0);
  for (auto itr(std::begin(values)); itr != std::end(values); ++itr) {
    size_t this_value_count(++value_and_count[*itr]);
    if (this_value_count > most_frequent) {
      most_frequent = this_value_count;
      most_frequent_itr = itr;
    }
  }

  if (value_and_count.empty())
    ThrowError(CommonErrors::unknown);

  if (value_and_count.size() == 1U)
    return *most_frequent_itr;

  if (value_and_count.size() == 2U && most_frequent == 1U)
    return (values[0] + values[1]) / 2;

  // Strip the first and last values if they only have a count of 1.
  if ((*std::begin(value_and_count)).second == 1U)
    value_and_count.erase(std::begin(value_and_count));
  if ((*(--std::end(value_and_count))).second == 1U)
    value_and_count.erase(--std::end(value_and_count));

  T total(0);
  size_t count(0);
  for (const auto& element : value_and_count) {
    total += element.first * element.second;
    count += element.second;
  }

  return total / count;
}

//PmidManagerMetadata MergePmidTotals(std::shared_ptr<GetPmidTotalsOp> op_data) {
//  // Remove invalid results
//  op_data->pmid_records.erase(
//      std::remove_if(std::begin(op_data->pmid_records),
//                     std::end(op_data->pmid_records),
//                     [&op_data](const PmidManagerMetadata& pmid_record) {
//                         return pmid_record.pmid_name->IsInitialised() &&
//                                pmid_record.pmid_name == op_data->kPmidAccountName;
//                     }),
//      std::end(op_data->pmid_records));

//  std::vector<int64_t> all_stored_counts, all_stored_total_size, all_lost_count,
//                       all_lost_total_size, all_claimed_available_size;
//  for (const auto& pmid_record : op_data->pmid_records) {
//    all_stored_counts.push_back(pmid_record.stored_count);
//    all_stored_total_size.push_back(pmid_record.stored_total_size);
//    all_lost_count.push_back(pmid_record.lost_count);
//    all_lost_total_size.push_back(pmid_record.lost_total_size);
//    all_claimed_available_size.push_back(pmid_record.claimed_available_size);
//  }

//  PmidManagerMetadata merged(op_data->kPmidAccountName);
//  merged.stored_count = Merge(all_stored_counts);
//  merged.stored_total_size = Merge(all_stored_total_size);
//  merged.lost_count = Merge(all_lost_count);
//  merged.lost_total_size = Merge(all_lost_total_size);
//  merged.claimed_available_size = Merge(all_claimed_available_size);
//  return merged;
//}

}  // unnamed namespace

MaidManagerService::MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing)
    : routing_(routing),
//      data_getter_(public_key_getter),
      group_db_(),
      accumulator_mutex_(),
      accumulator_(),
      dispatcher_(routing_, pmid),
      sync_create_accounts_(),
      sync_remove_accounts_(),
      sync_puts_(),
      sync_deletes_(),
      sync_register_pmids_(),
      sync_unregister_pmids_() {}

//void MaidManagerService::HandleMessage(const nfs::Message& message,
//                                       const routing::ReplyFunctor& reply_functor) {
//  ValidateGenericSender(message);
//  nfs::Reply reply(CommonErrors::success);
//  // TODO(Fraser#5#): 2013-07-25 - Uncomment once accummulator can handle non-Data messages
//  // {
//  //   std::lock_guard<std::mutex> lock(accumulator_mutex_);
//  //   if (accumulator_.CheckHandled(message, reply))
//  //     return reply_functor(reply.Serialise()->string());
//  // }

//  nfs::MessageAction action(message.data().action);
//  switch (action) {
//    case nfs::MessageAction::kRegisterPmid:
//      return HandlePmidRegistration(message, reply_functor);
//    case nfs::MessageAction::kSynchronise:
//      return HandleSync(message);
//    case nfs::MessageAction::kAccountTransfer:
//      return HandleAccountTransfer(message);
//    default:
//      LOG(kError) << "Unhandled Post action type";
//  }

//  reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
//  //SendReplyAndAddToAccumulator(message, reply_functor, reply);
//  reply_functor(reply.Serialise()->string());
//}

//void MaidManagerService::CheckSenderIsConnectedMaidNode(const nfs::Message& message) const {
//  if (!routing_.IsConnectedClient(message.source().node_id))
//    ThrowError(VaultErrors::permission_denied);
//  if (!FromClientMaid(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

//void MaidManagerService::CheckSenderIsConnectedMaidManager(const nfs::Message& message) const {
//  if (!routing_.IsConnectedVault(message.source().node_id))
//    ThrowError(VaultErrors::permission_denied);
//  if (!FromMaidManager(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

//void MaidManagerService::ValidateDataSender(const nfs::Message& message) const {
//  if (!ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//  CheckSenderIsConnectedMaidNode(message);
//}

//void MaidManagerService::ValidateGenericSender(const nfs::Message& message) const {
//  if (!ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);

//  if (message.data().action == nfs::MessageAction::kRegisterPmid ||
//      message.data().action == nfs::MessageAction::kUnregisterPmid) {
//    CheckSenderIsConnectedMaidNode(message);
//  } else {
//    CheckSenderIsConnectedMaidManager(message);
//  }
//}


// =============== Put/Delete data =================================================================

//void MaidManagerService::SendReplyAndAddToAccumulator(
//    const nfs::Message& message,
//    const routing::ReplyFunctor& reply_functor,
//    const nfs::Reply& reply) {
//  reply_functor(reply.Serialise()->string());
//  std::lock_guard<std::mutex> lock(accumulator_mutex_);
//  accumulator_.SetHandled(message, reply);
//}

//template<>
//void MaidManagerService::HandlePut<OwnerDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor) {
//  return HandleVersionMessage<OwnerDirectory>(message, reply_functor);
//}

//template<>
//void MaidManagerService::HandlePut<GroupDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor) {
//  return HandleVersionMessage<GroupDirectory>(message, reply_functor);
//}

//template<>
//void MaidManagerService::HandlePut<WorldDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor) {
//  return HandleVersionMessage<WorldDirectory>(message, reply_functor);
//}

// =============== Pmid registration ===============================================================

//void MaidManagerService::HandlePmidRegistration(const nfs::Message& message,
//                                                const routing::ReplyFunctor& reply_functor) {
//  NodeId source_id(message.source().node_id);

//  // TODO(Fraser#5#): 2013-04-22 - Validate Message signature.  Currently the Message does not have
//  //                  a signature applied, and the demuxer doesn't pass the signature down anyway.
//  nfs::PmidRegistration pmid_registration(nfs::PmidRegistration::serialised_type(NonEmptyString(
//      message.data().content.string())));
//  if (pmid_registration.maid_name()->string() != source_id.string())
//    return reply_functor(nfs::Reply(VaultErrors::permission_denied).Serialise()->string());

//  auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(pmid_registration, reply_functor));
////FIXME Prakash need to have utility to get public key
////  public_key_getter_.GetKey<passport::PublicMaid>(
////      pmid_registration.maid_name(),
////      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
////          ValidatePmidRegistration<passport::PublicMaid>(reply, pmid_registration.maid_name(),
////                                                         pmid_registration_op);
////      });
////  public_key_getter_.GetKey<passport::PublicPmid>(
////      pmid_registration.pmid_name(),
////      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
////          ValidatePmidRegistration<passport::PublicPmid>(reply, pmid_registration.pmid_name(),
////                                                         pmid_registration_op);
////      });
//}

//void MaidManagerService::FinalisePmidRegistration(
//    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
//  assert(pmid_registration_op->count == 2);
//  auto send_reply([&](const maidsafe_error& error)->void {
//      nfs::Reply reply(error);
//      pmid_registration_op->reply_functor(reply.Serialise()->string());
//  });

//  if (!pmid_registration_op->public_maid || !pmid_registration_op->public_pmid) {
//    LOG(kWarning) << "Failed to retrieve one or both of MAID and PMID";
//    return send_reply(maidsafe_error(VaultErrors::permission_denied));
//  }

//  try {
//    if (!pmid_registration_op->pmid_registration.Validate(*pmid_registration_op->public_maid,
//                                                          *pmid_registration_op->public_pmid)) {
//      LOG(kWarning) << "Failed to validate PmidRegistration";
//      return send_reply(maidsafe_error(VaultErrors::permission_denied));
//    }

//    if (pmid_registration_op->pmid_registration.unregister()) {
//      maid_account_handler_.UnregisterPmid(pmid_registration_op->public_maid->name(),
//                                           pmid_registration_op->public_pmid->name());
//    } else {
//      maid_account_handler_.RegisterPmid(pmid_registration_op->public_maid->name(),
//                                         pmid_registration_op->pmid_registration);
//    }
//    send_reply(maidsafe_error(CommonErrors::success));
//    UpdatePmidTotals(pmid_registration_op->public_maid->name());
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << "Failed to register new PMID: " << error.what();
//    send_reply(error);
//  }
//  catch(const std::exception& ex) {
//    LOG(kWarning) << "Failed to register new PMID: " << ex.what();
//    send_reply(maidsafe_error(CommonErrors::unknown));
//  }
//}

//void MaidManagerService::HandleCreateAccount(const MaidName& maid_name) {
//  CreateAccount(maid_name, can_create_account<>;
//}


// =============== Sync ============================================================================

void MaidManagerService::DoSync() {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_accounts_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_accounts_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_register_pmids_);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_unregister_pmids_);
}

//void MaidManagerService::HandleSync(const nfs::Message& /*message*/) {
//  protobuf::Sync proto_sync;
//  if (!proto_sync.ParseFromString(message.data().content.string()))
//    ThrowError(CommonErrors::parsing_error);

//  switch (proto_sync.action_type()) {
//    case ActionCreateAccount::kActionId: {
//      MaidManager::UnresolvedCreateAccount unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_create_accounts_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action) {
//        MaidManager::Metadata metadata;
//        group_db_.AddGroup(resolved_action->key.group_name, metadata);
//      }
//      break;
//    }
//    case ActionRemoveAccount::kActionId: {
//      MaidManager::UnresolvedRemoveAccount unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_remove_accounts_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action)
//        group_db_.DeleteGroup(resolved_action->key.group_name);
//      break;
//    }
//    case ActionMaidManagerPut::kActionId: {
//      MaidManager::UnresolvedPut unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action)
//        group_db_.Commit(resolved_action->key, resolved_action->action);
//      break;
//    }
//    case ActionMaidManagerDelete::kActionId: {
//      MaidManager::UnresolvedDelete unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action)
//        group_db_.Commit(resolved_action->key, resolved_action->action);
//      break;
//    }
//    case ActionRegisterPmid::kActionId: {
//      MaidManager::UnresolvedRegisterPmid unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_register_pmids_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action)
//        group_db_.Commit(resolved_action->key.group_name, resolved_action->action);
//      break;
//    }
//    case ActionUnregisterPmid::kActionId: {
//      MaidManager::UnresolvedUnregisterPmid unresolved_action(
//          proto_sync.serialised_unresolved_action(), message.source().node_id, routing_.kNodeId());
//      auto resolved_action(sync_unregister_pmids_.AddUnresolvedAction(unresolved_action));
//      if (resolved_action)
//        group_db_.Commit(resolved_action->key.group_name, resolved_action->action);
//      break;
//    }
//    default: {
//      assert(false);
//      LOG(kError) << "Unhandled action type";
//    }
//  }
//}


// =============== Account transfer ================================================================

//void MaidManagerService::TransferAccount(const MaidName& account_name,
//                                               const NodeId& new_node) {
//  protobuf::MaidManager maid_account;
//  maid_account.set_maid_name(account_name->string());
//  maid_account.set_serialised_account_details(
//      maid_account_handler_.GetSerialisedAccount(account_name)->string());
//  nfs_.TransferAccount(new_node, NonEmptyString(maid_account.SerializeAsString()));
//}

//void MaidManagerService::HandleAccountTransfer(const nfs::Message& message) {
//  protobuf::MaidManager maid_account;
//  NodeId source_id(message.source().node_id);
//  if (!maid_account.ParseFromString(message.data().content.string()))
//    return;

//  MaidName account_name(Identity(maid_account.maid_name()));
//  bool finished_all_transfers(
//      maid_account_handler_.ApplyAccountTransfer(account_name, source_id,
//          MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details()))));
//  if (finished_all_transfers)
//    UpdatePmidTotals(account_name);
//}


// =============== PMID totals =====================================================================

//void MaidManagerService::UpdatePmidTotals(const MaidName& account_name) {
//  auto pmid_names(maid_account_handler_.GetPmidNames(account_name));
//  for (const auto& pmid_name : pmid_names) {
//    auto op_data(std::make_shared<GetPmidTotalsOp>(account_name, pmid_name));
//    nfs_.RequestPmidTotals(pmid_name,
//                           [this, op_data](std::string serialised_reply) {
//                             UpdatePmidTotalsCallback(serialised_reply, op_data);
//                           });
//  }
//}

//void MaidManagerService::UpdatePmidTotalsCallback(const std::string& serialised_reply,
//                                                        std::shared_ptr<GetPmidTotalsOp> op_data) {
//  PmidManagerMetadata pmid_record;
//  try {
//    nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
//    if (reply.IsSuccess())
//      pmid_record = PmidManagerMetadata(PmidManagerMetadata::serialised_type(reply.data()));
//  }
//  catch(const std::exception& e) {
//    LOG(kWarning) << "Error updating PMID totals: " << e.what();
//  }

//  std::lock_guard<std::mutex> lock(op_data->mutex);
//  op_data->pmid_records.push_back(pmid_record);
//  assert(op_data->pmid_records.size() <= routing::Parameters::node_group_size);
//  if (op_data->pmid_records.size() != routing::Parameters::node_group_size)
//    return;

//  try {
//    auto pmid_record(MergePmidTotals(op_data));
//    maid_account_handler_.UpdatePmidTotals(op_data->kMaidManagerName, pmid_record);
//  }
//  catch(const std::exception& e) {
//    LOG(kWarning) << "Error updating PMID totals: " << e.what();
//  }
//}

//void MaidManagerService::HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change) {
//  auto account_names(maid_account_handler_.GetAccountNames());
//  auto itr(std::begin(account_names));
//  while (itr != std::end(account_names)) {
//    auto check_holders_result(matrix_change->CheckHolders(NodeId((*itr)->string())));
//    // Delete accounts for which this node is no longer responsible.
//    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
//      maid_account_handler_.DeleteAccount(*itr);
//      itr = account_names.erase(itr);
//      continue;
//    }

//    // Replace old_node(s) in sync object and send AccountTransfer to new node(s).
//    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
//    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
//      maid_account_handler_.ReplaceNodeInSyncList(*itr, check_holders_result.old_holders[i],
//                                                  check_holders_result.new_holders[i]);
//      TransferAccount(*itr, check_holders_result.new_holders[i]);
//    }

//    ++itr;
//  }
//}


template<>
void MaidManagerService::HandleMessage(
    const nfs::PutRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::PutRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::PutResponseFromDataManagerToMaidManager& message,
    const typename nfs::PutResponseFromDataManagerToMaidManager::Sender& sender,
    const typename nfs::PutResponseFromDataManagerToMaidManager::Receiver& receiver) {
  typedef nfs::PutResponseFromDataManagerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::DeleteRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::PutVersionRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::CreateAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::RemoveAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::RegisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::UnregisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::UnregisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::GetPmidHealthRequestFromMaidNodeToMaidManager& message,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::GetPmidHealthRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(RequiredRequests(message)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::SynchroniseFromMaidManagerToMaidManager& message,
    const typename nfs::SynchroniseFromMaidManagerToMaidManager::Sender& sender,
    const typename nfs::SynchroniseFromMaidManagerToMaidManager::Receiver& /*receiver*/) {
    protobuf::Sync proto_sync;
    if (!proto_sync.ParseFromString(message.contents->content.string()))
      ThrowError(CommonErrors::parsing_error);
    switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
      case ActionMaidManagerPut::kActionId: {
        MaidManager::UnresolvedPut unresolved_action(
            proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
        auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
        if (resolved_action)
          group_db_.Commit(resolved_action->key, resolved_action->action);
        break;
      }
//      case ActionCreateAccount::kActionId: {
//        MaidManager::UnresolvedCreateAccount unresolved_action(
//            proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
//        auto resolved_action(sync_create_accounts_.AddUnresolvedAction(unresolved_action));
//        if (resolved_action) {
//          MaidManager::Metadata metadata;
//          group_db_.AddGroup(resolved_action->key.group_name, metadata);
//        }
//        break;
//      }

      default: {
        assert(false);
        LOG(kError) << "Unhandled action type";
      }
    }
}

}  // namespace vault

}  // namespace maidsafe
