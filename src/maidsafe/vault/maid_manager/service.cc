/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/maid_manager/service.h"

#include <string>

#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/data_types/owner_directory.h"
#include "maidsafe/data_types/group_directory.h"
#include "maidsafe/data_types/world_directory.h"

#include "maidsafe/vault/maid_manager/helpers.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/sync.h"


namespace maidsafe {

namespace vault {

namespace detail {

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

MaidName GetMaidAccountName(const nfs::Message& message) {
  return MaidName(Identity(message.source().node_id.string()));
}

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

PmidManagerMetadata MergePmidTotals(std::shared_ptr<GetPmidTotalsOp> op_data) {
  // Remove invalid results
  op_data->pmid_records.erase(
      std::remove_if(std::begin(op_data->pmid_records),
                     std::end(op_data->pmid_records),
                     [&op_data](const PmidManagerMetadata& pmid_record) {
                         return pmid_record.pmid_name->IsInitialised() &&
                                pmid_record.pmid_name == op_data->kPmidAccountName;
                     }),
      std::end(op_data->pmid_records));

  std::vector<int64_t> all_stored_counts, all_stored_total_size, all_lost_count,
                       all_lost_total_size, all_claimed_available_size;
  for (const auto& pmid_record : op_data->pmid_records) {
    all_stored_counts.push_back(pmid_record.stored_count);
    all_stored_total_size.push_back(pmid_record.stored_total_size);
    all_lost_count.push_back(pmid_record.lost_count);
    all_lost_total_size.push_back(pmid_record.lost_total_size);
    all_claimed_available_size.push_back(pmid_record.claimed_available_size);
  }

  PmidManagerMetadata merged(op_data->kPmidAccountName);
  merged.stored_count = Merge(all_stored_counts);
  merged.stored_total_size = Merge(all_stored_total_size);
  merged.lost_count = Merge(all_lost_count);
  merged.lost_total_size = Merge(all_lost_total_size);
  merged.claimed_available_size = Merge(all_claimed_available_size);
  return merged;
}

}  // unnamed namespace



const int MaidManagerService::kPutRepliesSuccessesRequired_(3);
const int MaidManagerService::kDefaultPaymentFactor_(4);


MaidManagerService::MaidManagerService(const passport::Pmid& pmid,
                                       routing::Routing& routing,
                                       nfs::PublicKeyGetter& public_key_getter,
                                       Db& db)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      group_db_(),
      accumulator_mutex_(),
      accumulator_(),
      nfs_(routing, pmid),
      sync_create_accounts_(routing.kNodeId()),
      sync_remove_accounts_(routing.kNodeId()),
      sync_puts_(routing.kNodeId()),
      sync_deletes_(routing.kNodeId()),
      sync_register_pmids_(routing.kNodeId()),
      sync_unregister_pmids_(routing.kNodeId()) {}

void MaidManagerService::HandleMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  ValidateGenericSender(message);
  nfs::Reply reply(CommonErrors::success);
  // TODO(Fraser#5#): 2013-07-25 - Uncomment once accummulator can handle non-Data messages
  // {
  //   std::lock_guard<std::mutex> lock(accumulator_mutex_);
  //   if (accumulator_.CheckHandled(message, reply))
  //     return reply_functor(reply.Serialise()->string());
  // }

  nfs::MessageAction action(message.data().action);
  switch (action) {
    case nfs::MessageAction::kRegisterPmid:
      return HandlePmidRegistration(message, reply_functor);
    case nfs::MessageAction::kSynchronise:
      return HandleSync(message);
    case nfs::MessageAction::kAccountTransfer:
      return HandleAccountTransfer(message);
    default:
      LOG(kError) << "Unhandled Post action type";
  }

  reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
  //SendReplyAndAddToAccumulator(message, reply_functor, reply);
  reply_functor(reply.Serialise()->string());
}

void MaidManagerService::CheckSenderIsConnectedMaidNode(const nfs::Message& message) const {
  if (!routing_.IsConnectedClient(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!FromClientMaid(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MaidManagerService::CheckSenderIsConnectedMaidManager(const nfs::Message& message) const {
  if (!routing_.IsConnectedVault(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!FromMaidManager(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MaidManagerService::ValidateDataSender(const nfs::Message& message) const {
  if (!ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
  CheckSenderIsConnectedMaidNode(message);
}

void MaidManagerService::ValidateGenericSender(const nfs::Message& message) const {
  if (!ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);

  if (message.data().action == nfs::MessageAction::kRegisterPmid ||
      message.data().action == nfs::MessageAction::kUnregisterPmid) {
    CheckSenderIsConnectedMaidNode(message);
  } else {
    CheckSenderIsConnectedMaidManager(message);
  }
}


// =============== Put/Delete data =================================================================

void MaidManagerService::SendReplyAndAddToAccumulator(
    const nfs::Message& message,
    const routing::ReplyFunctor& reply_functor,
    const nfs::Reply& reply) {
  reply_functor(reply.Serialise()->string());
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(message, reply);
}

template<>
void MaidManagerService::HandlePut<OwnerDirectory>(const nfs::Message& message,
                                                   const routing::ReplyFunctor& reply_functor) {
  return HandleVersionMessage<OwnerDirectory>(message, reply_functor);
}

template<>
void MaidManagerService::HandlePut<GroupDirectory>(const nfs::Message& message,
                                                   const routing::ReplyFunctor& reply_functor) {
  return HandleVersionMessage<GroupDirectory>(message, reply_functor);
}

template<>
void MaidManagerService::HandlePut<WorldDirectory>(const nfs::Message& message,
                                                   const routing::ReplyFunctor& reply_functor) {
  return HandleVersionMessage<WorldDirectory>(message, reply_functor);
}

// =============== Pmid registration ===============================================================

void MaidManagerService::HandlePmidRegistration(const nfs::Message& message,
                                                      const routing::ReplyFunctor& reply_functor) {
  NodeId source_id(message.source().node_id);

  // TODO(Fraser#5#): 2013-04-22 - Validate Message signature.  Currently the Message does not have
  //                  a signature applied, and the demuxer doesn't pass the signature down anyway.
  nfs::PmidRegistration pmid_registration(nfs::PmidRegistration::serialised_type(NonEmptyString(
      message.data().content.string())));
  if (pmid_registration.maid_name()->string() != source_id.string())
    return reply_functor(nfs::Reply(VaultErrors::permission_denied).Serialise()->string());

  auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(pmid_registration, reply_functor));

  public_key_getter_.GetKey<passport::PublicMaid>(
      pmid_registration.maid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidatePmidRegistration<passport::PublicMaid>(reply, pmid_registration.maid_name(),
                                                         pmid_registration_op);
      });
  public_key_getter_.GetKey<passport::PublicPmid>(
      pmid_registration.pmid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidatePmidRegistration<passport::PublicPmid>(reply, pmid_registration.pmid_name(),
                                                         pmid_registration_op);
      });
}

void MaidManagerService::FinalisePmidRegistration(
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
  assert(pmid_registration_op->count == 2);
  auto send_reply([&](const maidsafe_error& error)->void {
      nfs::Reply reply(error);
      pmid_registration_op->reply_functor(reply.Serialise()->string());
  });

  if (!pmid_registration_op->public_maid || !pmid_registration_op->public_pmid) {
    LOG(kWarning) << "Failed to retrieve one or both of MAID and PMID";
    return send_reply(maidsafe_error(VaultErrors::permission_denied));
  }

  try {
    if (!pmid_registration_op->pmid_registration.Validate(*pmid_registration_op->public_maid,
                                                          *pmid_registration_op->public_pmid)) {
      LOG(kWarning) << "Failed to validate PmidRegistration";
      return send_reply(maidsafe_error(VaultErrors::permission_denied));
    }

    if (pmid_registration_op->pmid_registration.unregister()) {
      maid_account_handler_.UnregisterPmid(pmid_registration_op->public_maid->name(),
                                           pmid_registration_op->public_pmid->name());
    } else {
      maid_account_handler_.RegisterPmid(pmid_registration_op->public_maid->name(),
                                         pmid_registration_op->pmid_registration);
    }
    send_reply(maidsafe_error(CommonErrors::success));
    UpdatePmidTotals(pmid_registration_op->public_maid->name());
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "Failed to register new PMID: " << error.what();
    send_reply(error);
  }
  catch(const std::exception& ex) {
    LOG(kWarning) << "Failed to register new PMID: " << ex.what();
    send_reply(maidsafe_error(CommonErrors::unknown));
  }
}


// =============== Sync ============================================================================

void MaidManagerService::DoSync(const MaidName& account_name) {
  auto serialised_sync_data(maid_account_handler_.GetSyncData(account_name));
  if (!serialised_sync_data.IsInitialised())  // Nothing to sync
    return;

  protobuf::Sync proto_sync;
  proto_sync.set_account_name(account_name->string());
  proto_sync.set_serialised_unresolved_actions(serialised_sync_data.string());

  nfs_.Sync(account_name, NonEmptyString(proto_sync.SerializeAsString()));
  // TODO(Fraser#5#): 2013-05-03 - Check this is correct place to increment sync attempt counter.
  maid_account_handler_.IncrementSyncAttempts(account_name);
}

void MaidManagerService::HandleSync(const nfs::Message& message) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.data().content.string())) {
    LOG(kError) << "Error parsing kSynchronise message.";
    return;
  }
  maid_account_handler_.ApplySyncData(MaidName(Identity(proto_sync.account_name())),
                                      NonEmptyString(proto_sync.serialised_unresolved_actions()));
}


// =============== Account transfer ================================================================

void MaidManagerService::TransferAccount(const MaidName& account_name,
                                               const NodeId& new_node) {
  protobuf::MaidManager maid_account;
  maid_account.set_maid_name(account_name->string());
  maid_account.set_serialised_account_details(
      maid_account_handler_.GetSerialisedAccount(account_name)->string());
  nfs_.TransferAccount(new_node, NonEmptyString(maid_account.SerializeAsString()));
}

void MaidManagerService::HandleAccountTransfer(const nfs::Message& message) {
  protobuf::MaidManager maid_account;
  NodeId source_id(message.source().node_id);
  if (!maid_account.ParseFromString(message.data().content.string()))
    return;

  MaidName account_name(Identity(maid_account.maid_name()));
  bool finished_all_transfers(
      maid_account_handler_.ApplyAccountTransfer(account_name, source_id,
          MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details()))));
  if (finished_all_transfers)
    UpdatePmidTotals(account_name);
}


// =============== PMID totals =====================================================================

void MaidManagerService::UpdatePmidTotals(const MaidName& account_name) {
  auto pmid_names(maid_account_handler_.GetPmidNames(account_name));
  for (const auto& pmid_name : pmid_names) {
    auto op_data(std::make_shared<GetPmidTotalsOp>(account_name, pmid_name));
    nfs_.RequestPmidTotals(pmid_name,
                           [this, op_data](std::string serialised_reply) {
                             UpdatePmidTotalsCallback(serialised_reply, op_data);
                           });
  }
}

void MaidManagerService::UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                                        std::shared_ptr<GetPmidTotalsOp> op_data) {
  PmidManagerMetadata pmid_record;
  try {
    nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
    if (reply.IsSuccess())
      pmid_record = PmidManagerMetadata(PmidManagerMetadata::serialised_type(reply.data()));
  }
  catch(const std::exception& e) {
    LOG(kWarning) << "Error updating PMID totals: " << e.what();
  }

  std::lock_guard<std::mutex> lock(op_data->mutex);
  op_data->pmid_records.push_back(pmid_record);
  assert(op_data->pmid_records.size() <= routing::Parameters::node_group_size);
  if (op_data->pmid_records.size() != routing::Parameters::node_group_size)
    return;

  try {
    auto pmid_record(MergePmidTotals(op_data));
    maid_account_handler_.UpdatePmidTotals(op_data->kMaidManagerName, pmid_record);
  }
  catch(const std::exception& e) {
    LOG(kWarning) << "Error updating PMID totals: " << e.what();
  }
}

void MaidManagerService::HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change) {
  auto account_names(maid_account_handler_.GetAccountNames());
  auto itr(std::begin(account_names));
  while (itr != std::end(account_names)) {
    auto check_holders_result(matrix_change->CheckHolders(NodeId((*itr)->string())));
    // Delete accounts for which this node is no longer responsible.
    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      maid_account_handler_.DeleteAccount(*itr);
      itr = account_names.erase(itr);
      continue;
    }

    // Replace old_node(s) in sync object and send AccountTransfer to new node(s).
    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
      maid_account_handler_.ReplaceNodeInSyncList(*itr, check_holders_result.old_holders[i],
                                                  check_holders_result.new_holders[i]);
      TransferAccount(*itr, check_holders_result.new_holders[i]);
    }

    ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe
