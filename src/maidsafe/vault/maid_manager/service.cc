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
#include "maidsafe/nfs/vault/pmid_registration.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_update_pmid_health.h"
#include "maidsafe/vault/maid_manager/action_reference_counts.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/helpers.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/maid_manager/action_reference_count.h"

namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
int32_t EstimateCost(const T&) {
  return 0;
}

template <>
int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&) {
  return 0;
}

template <>
int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&) {
  return 0;
}

template <>
int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&) {
  return 0;
}

// MaidName GetMaidAccountName(const nfs::Message& message) {
//  return MaidName(Identity(message.source().node_id.string()));
// }

}  // namespace detail

namespace {

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMaidManager;
}

template <typename T>
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
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::unknown));

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

// PmidManagerMetadata MergePmidTotals(std::shared_ptr<GetPmidTotalsOp> op_data) {
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
// }

}  // unnamed namespace

MaidManagerService::MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       nfs_client::DataGetter& data_getter,
                                       const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      data_getter_(data_getter),
      group_db_(UniqueDbPath(vault_root_dir)),
      accumulator_mutex_(),
      mutex_(),
      nfs_accumulator_(),
      vault_accumulator_(),
      dispatcher_(routing_, pmid),
      sync_create_accounts_(NodeId(pmid.name()->string())),
      sync_remove_accounts_(NodeId(pmid.name()->string())),
      sync_puts_(NodeId(pmid.name()->string())),
      sync_deletes_(NodeId(pmid.name()->string())),
      sync_register_pmids_(NodeId(pmid.name()->string())),
      sync_unregister_pmids_(NodeId(pmid.name()->string())),
      sync_update_pmid_healths_(NodeId(pmid.name()->string())),
      sync_increment_reference_counts_(NodeId(pmid.name()->string())),
      sync_decrement_reference_counts_(NodeId(pmid.name()->string())),
      account_transfer_(),
      pending_account_mutex_(),
      pending_account_map_() {}

// =============== Maid Account Creation ===========================================================

void MaidManagerService::HandleCreateMaidAccount(const passport::PublicMaid& public_maid,
                                                 const passport::PublicAnmaid& public_anmaid,
                                                 nfs::MessageId message_id) {
  MaidName account_name(public_maid.name());
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  // If Account exists
  try {
    group_db_.GetMetadata(account_name);
    LOG(kError) << "account already exists in group_db_";
    dispatcher_.SendCreateAccountResponse(account_name,
                                          maidsafe_error(VaultErrors::account_already_exists),
                                          message_id);
    return;
  } catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "db errors" << boost::diagnostic_information(error);
      throw;
    }
  }

  pending_account_map_.insert(std::make_pair(message_id,
                                             MaidAccountCreationStatus(public_maid.name(),
                                             public_anmaid.name())));
  dispatcher_.SendPutRequest(account_name, public_maid, PmidName(Identity(NodeId().string())),
                             message_id);
  dispatcher_.SendPutRequest(account_name, public_anmaid, PmidName(Identity(NodeId().string())),
                             message_id);
}

template <>
void MaidManagerService::HandlePutResponse<passport::PublicMaid>(const MaidName& maid_name,
    const typename passport::PublicMaid::Name& data_name, int32_t ,
    nfs::MessageId message_id) {
  std::lock_guard<std::mutex> lock(pending_account_mutex_);
  auto pending_account_itr(pending_account_map_.find(message_id));
  if (pending_account_itr == pending_account_map_.end()) {
    // The below assert should be removed as the message may be received by any joining vault
    // assert(false && "not such a pending account request");
    LOG(kInfo) << "unexpected PublicMaid put response";
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
    const typename passport::PublicAnmaid::Name& data_name, int32_t, nfs::MessageId message_id) {
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
//    sync_create_accounts_.AddLocalAction(
//        MaidManager::UnresolvedCreateAccount(maid_name, ActionCreateAccount(message_id),
//                                             routing_.kNodeId()));
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
  MaidManager::Metadata metadata;
  try {
    group_db_.AddGroup(synced_action->key.group_name(), metadata);
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::account_already_exists)) {
      throw;
    } else {
      LOG(kVerbose) << "HandleSyncedCreateMaidAccount:already exist "
                    << DebugId(synced_action->key.group_name());
    }
  }
  dispatcher_.SendCreateAccountResponse(synced_action->key.group_name(),
                                        maidsafe_error(CommonErrors::success),
                                        synced_action->action.kMessageId);
}

void MaidManagerService::HandleSyncedRemoveMaidAccount(
    std::unique_ptr<MaidManager::UnresolvedRemoveAccount>&& synced_action) {
  group_db_.DeleteGroup(synced_action->key.group_name());
  dispatcher_.SendRemoveAccountResponse(synced_action->key.group_name(),
                                        maidsafe_error(CommonErrors::success),
                                        synced_action->action.kMessageId);
}

// =============== Pmid registration ===============================================================

void MaidManagerService::HandlePmidRegistration(
    const nfs_vault::PmidRegistration& pmid_registration, nfs::MessageId message_id) {
// FIXME This should be implemented in validate method
//  if (pmid_registration.maid_name() != source_maid_name)
//    return;
//  sync_register_pmids_.AddLocalAction(
//      MaidManager::UnresolvedRegisterPmid(pmid_registration.maid_name(),
//          ActionMaidManagerRegisterPmid(pmid_registration), routing_.kNodeId()));
  LOG(kVerbose) << "MaidManagerService::HandlePmidRegistration id: " << message_id;
  DoSync(MaidManager::UnresolvedRegisterPmid(
      MaidManager::MetadataKey(pmid_registration.maid_name()),
      ActionMaidManagerRegisterPmid(pmid_registration, message_id), routing_.kNodeId()));
}

void MaidManagerService::HandlePmidUnregistration(const MaidName& maid_name,
                                                  const PmidName& pmid_name) {
  // BEFORE_RELEASE: further action may be required
//  sync_unregister_pmids_.AddLocalAction(
//      MaidManager::UnresolvedUnregisterPmid(maid_name,
//          ActionMaidManagerUnregisterPmid(pmid_name), routing_.kNodeId()));
  DoSync(MaidManager::UnresolvedUnregisterPmid(MaidManager::MetadataKey(maid_name),
      ActionMaidManagerUnregisterPmid(pmid_name), routing_.kNodeId()));
}

void MaidManagerService::HandleSyncedPmidRegistration(
    std::unique_ptr<MaidManager::UnresolvedRegisterPmid>&& synced_action) {
  // Get keys
  // BEFORE_RELEASE future.then still have confirmed bug :
  // http://stackoverflow.com/questions/14829881/bad-access-in-boostfuture-then-after-accessing-given-future
  // The following commented out code doesn't work as expected,
  // Have to use two individual blocking calling, which may hang if Get() doesn't return
  // May be able to use future.then before release

//   auto maid_future = data_getter_.Get(synced_action->action.kPmidRegistration.maid_name(),
//                                       std::chrono::seconds(10));
//   auto pmid_future = data_getter_.Get(synced_action->action.kPmidRegistration.pmid_name(),
//                                       std::chrono::seconds(10));
//
//   auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(std::move(synced_action)));
//
//   auto maid_future_then = maid_future.then(
//       [pmid_registration_op, this](boost::future<passport::PublicMaid>& future) {
//           try {
//             std::unique_ptr<passport::PublicMaid> public_maid(new passport::PublicMaid(
//                                                                   future.get()));
//             ValidatePmidRegistration(std::move(public_maid), pmid_registration_op);
//           } catch(const std::exception& e) {
//             LOG(kError) << e.what();
//           }
//       });
//
//   auto pmid_future_then = pmid_future.then(
//       [pmid_registration_op, this](boost::future<passport::PublicPmid>& future) {
//           try {
//             std::unique_ptr<passport::PublicPmid> public_pmid(new passport::PublicPmid(
//                                                                   future.get()));
//             ValidatePmidRegistration(std::move(public_pmid), pmid_registration_op);
//           } catch(const std::exception& e) {
//             LOG(kError) << e.what();
//           }
//       });
//   boost::wait_for_all(maid_future_then, pmid_future_then);
  LOG(kVerbose) << "MaidManagerService::HandleSyncedPmidRegistration";
  nfs::MessageId message_id(synced_action->action.message_id);
  auto maid_future = data_getter_.Get(synced_action->action.pmid_registration.maid_name(),
                                      std::chrono::seconds(10));
  auto pmid_future = data_getter_.Get(synced_action->action.pmid_registration.pmid_name(),
                                      std::chrono::seconds(10));

  auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(std::move(synced_action)));

  try {
    std::unique_ptr<passport::PublicPmid> public_pmid(new passport::PublicPmid(pmid_future.get()));
    LOG(kVerbose) << "MaidManagerService::HandleSyncedPmidRegistration got public_pmid";
    ValidatePmidRegistration(std::move(public_pmid), pmid_registration_op, message_id);
    std::unique_ptr<passport::PublicMaid> public_maid(new passport::PublicMaid(maid_future.get()));
    LOG(kVerbose) << "MaidManagerService::HandleSyncedPmidRegistration got public_maid";
    ValidatePmidRegistration(std::move(public_maid), pmid_registration_op, message_id);
  } catch(const std::exception& e) {
    LOG(kError) << "MaidManagerService::HandleSyncedPmidRegistration raised exception "
                << boost::diagnostic_information(e);
  }
}

void MaidManagerService::HandleSyncedPmidUnregistration(
    std::unique_ptr<MaidManager::UnresolvedUnregisterPmid>&& synced_action) {
// BEFORE_RELEASE : This may require more actions to complete (unregister pmidnode from pmidmanager)
  try {
    group_db_.Commit(synced_action->key.group_name(), synced_action->action);
  }
  catch (const maidsafe_error& error) {
    LOG(kWarning) << "MaidManagerService::HandleSyncedPmidUnregistration failed";
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

void MaidManagerService::FinalisePmidRegistration(
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op, nfs::MessageId message_id) {
  assert(pmid_registration_op->count == 2);
  maidsafe_error return_code(CommonErrors::success);

  if (!pmid_registration_op->public_maid || !pmid_registration_op->public_pmid) {
    LOG(kWarning) << "Failed to retrieve one or both of MAID and PMID";
    return;
  }

  try {
    if (!pmid_registration_op->synced_action->action.pmid_registration.Validate(
            *pmid_registration_op->public_maid, *pmid_registration_op->public_pmid)) {
      LOG(kWarning) << "Failed to validate PmidRegistration";
      return;
    }

    if (pmid_registration_op->synced_action->action.pmid_registration.unregister()) {
      group_db_.Commit(pmid_registration_op->synced_action->key.group_name(),
                       pmid_registration_op->synced_action->action);
    } else {
      group_db_.Commit(pmid_registration_op->synced_action->key.group_name(),
                       pmid_registration_op->synced_action->action);
      dispatcher_.SendCreatePmidAccountRequest(*pmid_registration_op->public_maid,
                                               *pmid_registration_op->public_pmid);
    }
  }
  catch(const maidsafe_error& error) {
    return_code = error;
    LOG(kWarning) << "Failed to register new PMID: " << boost::diagnostic_information(error);
  }
  catch(const std::exception& ex) {
    return_code = MakeError(CommonErrors::unknown);
    LOG(kWarning) << "Failed to register new PMID: " << boost::diagnostic_information(ex);
  }
  dispatcher_.SendRegisterPmidResponse(pmid_registration_op->public_maid->name(), return_code,
                                       message_id);
  LOG(kInfo) << "PmidRegistration Finalised";
}

void MaidManagerService::HandleSyncedUpdatePmidHealth(
    std::unique_ptr<MaidManager::UnresolvedUpdatePmidHealth>&& synced_action_update_pmid_health) {
  LOG(kVerbose) << "MaidManagerService::HandleSyncedUpdatePmidHealth";
  try {
    group_db_.Commit(synced_action_update_pmid_health->key.group_name(),
                     synced_action_update_pmid_health->action);
  }
  catch (const maidsafe_error& error) {
    LOG(kWarning) << "MaidManagerService::HandleSyncedUpdatePmidHealth failed";
    if (error.code() != make_error_code(CommonErrors::no_such_element))
      throw;
  }
}

// =============== Put/Delete data =================================================================
void MaidManagerService::HandleSyncedPutResponse(
    std::unique_ptr<MaidManager::UnresolvedPut>&& synced_action_put) {
  // BEFORE_RELEASE difference process for account_transfer (avoiding double hash)
  ObfuscateKey(synced_action_put->key);
  try {
    group_db_.Commit(synced_action_put->key, synced_action_put->action);
  }
  catch (const maidsafe_error& error) {
    LOG(kWarning) << "MaidManagerService::HandleSyncedPutResponse failed";
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

void MaidManagerService::HandleSyncedDelete(
    std::unique_ptr<MaidManager::UnresolvedDelete>&& synced_action_delete) {
  // BEFORE_RELEASE difference process for account_transfer (avoiding double hash)
  nfs_vault::DataName data_name(synced_action_delete->key.type,
                                synced_action_delete->key.name);
  ObfuscateKey(synced_action_delete->key);
  try {
    auto value(group_db_.Commit(synced_action_delete->key, synced_action_delete->action));
    // nullptr will be returned in case of reducing count only
    if (value)
      dispatcher_.SendDeleteRequest(synced_action_delete->key.group_name(),
                                    data_name,
                                    synced_action_delete->action.kMessageId);
    else
      LOG(kInfo) << "DeleteRequest not passed down to DataManager";
  } catch (const maidsafe_error& error) {
    LOG(kError) << "MaidManagerService::HandleSyncedDelete commiting error: "
                << boost::diagnostic_information(error);
    if (error.code() != make_error_code(CommonErrors::no_such_element) &&
        error.code() != make_error_code(VaultErrors::no_such_account)) {
      throw;
    } else {
      // BEFORE_RELEASE trying to delete something not belongs to client shall get muted
      VLOG(nfs::Persona::kMaidManager, VisualiserAction::kBlockDeleteRequest, data_name.raw_name);
    }
  }
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

// =============== PMID totals =====================================================================

void MaidManagerService::HandlePmidHealthRequest(
    const MaidName& maid_name, const PmidName& pmid_node, nfs::MessageId message_id) {
  try {
    group_db_.GetMetadata(maid_name);
    dispatcher_.SendPmidHealthRequest(maid_name, pmid_node, message_id);
  }
  catch (const maidsafe_error& error) {
    LOG(kError) << "MaidManagerService::HandlePmidHealthRequest faied: "
                << boost::diagnostic_information(error);
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

void MaidManagerService::HandlePmidHealthResponse(const MaidName& maid_name,
    const std::string& serialised_pmid_health, maidsafe_error& return_code,
    nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerService::HandleHealthResponse to " << HexSubstr(maid_name->string());
  try {
    PmidManagerMetadata pmid_health(serialised_pmid_health);
    LOG(kVerbose) << "PmidManagerMetadata available size " << pmid_health.claimed_available_size;
    if (return_code.code() == make_error_code(CommonErrors::success)) {
      DoSync(MaidManager::UnresolvedUpdatePmidHealth(MaidManager::MetadataKey(maid_name),
          ActionMaidManagerUpdatePmidHealth(pmid_health), routing_.kNodeId()));
    }
    dispatcher_.SendPmidHealthResponse(maid_name, pmid_health.claimed_available_size,
                                       return_code, message_id);
  } catch(std::exception& e) {
    LOG(kError) << "Error handling Health Response to " << HexSubstr(maid_name->string())
                << " with exception of " << boost::diagnostic_information(e);
  }
}

void MaidManagerService::HandleChurnEvent(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
//   if (close_nodes_change->lost_nodes().size() != 0) {
//     LOG(kVerbose) << "MaidManagerService::HandleChurnEvent";
//     close_nodes_change->Print();
//   }
  std::lock_guard<std::mutex> lock(mutex_);
  if (stopped_)
    return;
  GroupDb<MaidManager>::TransferInfo transfer_info(group_db_.GetTransferInfo(close_nodes_change));
  for (auto& transfer : transfer_info)
    TransferAccount(transfer.first, transfer.second);
}

void MaidManagerService::TransferAccount(const NodeId& dest,
    const std::vector<GroupDb<MaidManager>::Contents>& accounts) {
  for (auto& account : accounts) {
    // If account just received, shall not pass it out as may under a startup procedure
    // i.e. existing MM will be seen as new_node in close_nodes_change
    if (account_transfer_.CheckHandled(routing::GroupId(NodeId(account.group_name->string())))) {
      LOG(kInfo) << "MaidManager account " << HexSubstr(account.group_name->string())
                 << " just received";
      continue;
    }
    VLOG(nfs::Persona::kMaidManager, VisualiserAction::kAccountTransfer, account.group_name,
         Identity{ dest.string() });
    try {
      std::vector<std::string> actions;
      actions.push_back(account.metadata.Serialise());
      for (auto& kv : account.kv_pairs) {
        protobuf::MaidManagerKeyValuePair kv_msg;
          kv_msg.set_key(kv.first.Serialise());
          kv_msg.set_value(kv.second.Serialise());
          actions.push_back(kv_msg.SerializeAsString());
      }
      nfs::MessageId message_id(HashStringToMessageId(account.group_name->string()));
      MaidManager::UnresolvedAccountTransfer account_transfer(
          account.group_name, message_id, actions);
      dispatcher_.SendAccountTransfer(dest, account.group_name,
                                      message_id, account_transfer.Serialise());
      LOG(kVerbose) << "MaidManager sent to " << HexSubstr(dest.string())
                    << " with account " << account.Print();
    } catch(...) {
      // the normal problem is metadata hasn't been populated
      LOG(kError) << "MaidManagerService::TransferAccount account info error";
    }
  }
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
    const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::DeleteRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
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
    const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::RegisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::UnregisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::UnregisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::PmidHealthRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PmidHealthRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PmidHealthRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::PmidHealthRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const PmidHealthResponseFromPmidManagerToMaidManager& message,
    const typename PmidHealthResponseFromPmidManagerToMaidManager::Sender& sender,
    const typename PmidHealthResponseFromPmidManagerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef PmidHealthResponseFromPmidManagerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, VaultAccumulator>(
      vault_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                            return this->ValidateSender(message, sender);
                          },
      VaultAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::IncrementReferenceCountsFromMaidNodeToMaidManager& message,
    const typename nfs::IncrementReferenceCountsFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::IncrementReferenceCountsFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::IncrementReferenceCountsFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService, MessageType, NfsAccumulator>(
      nfs_accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                          return this->ValidateSender(message, sender);
                        },
      NfsAccumulator::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::DecrementReferenceCountsFromMaidNodeToMaidManager& message,
    const typename nfs::DecrementReferenceCountsFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DecrementReferenceCountsFromMaidNodeToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef nfs::DecrementReferenceCountsFromMaidNodeToMaidManager MessageType;
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
    case ActionMaidManagerRegisterPmid::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionRegisterPmid";
      MaidManager::UnresolvedRegisterPmid unresolved_action(
        proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_register_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedPmidRegistration";
        HandleSyncedPmidRegistration(std::move(resolved_action));
      }
      break;
    }
    case ActionMaidManagerUnregisterPmid::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionUnregisterPmid";
      MaidManager::UnresolvedUnregisterPmid unresolved_action(
        proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_unregister_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedPmidUnegistration";
        HandleSyncedPmidUnregistration(std::move(resolved_action));
      }
      break;
    }
    case ActionMaidManagerUpdatePmidHealth::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager ActionUpdatePmidHealth";
      MaidManager::UnresolvedUpdatePmidHealth unresolved_action(
        proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_update_pmid_healths_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedUpdatePmidHealth";
        HandleSyncedUpdatePmidHealth(std::move(resolved_action));
      }
      break;
    }
    case ActionMaidManagerIncrementReferenceCounts::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager IncrementReferenceCounts";
      MaidManager::UnresolvedIncrementReferenceCounts unresolved_action(
        proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_increment_reference_counts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager HandleSyncedIncReferenceCounts";
        HandleSyncedIncrementReferenceCounts(std::move(resolved_action));
      }
      break;
    }
    case ActionMaidManagerDecrementReferenceCounts::kActionId: {
      LOG(kVerbose) << "SynchroniseFromMaidManagerToMaidManager DecrementReferenceCounts";
      MaidManager::UnresolvedDecrementReferenceCounts unresolved_action(
        proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_decrement_reference_counts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        LOG(kInfo) << "SynchroniseFromMaidManagerToMaidManager SyncedDecReferenceCounts";
        HandleSyncedDecrementReferenceCounts(std::move(resolved_action));
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

void MaidManagerService::HandleIncrementReferenceCounts(const MaidName& maid_name,
                                      const nfs_vault::DataNames& data_names) {
  if (CheckDataNamesExist(maid_name, data_names)) {
    DoSync(MaidManager::UnresolvedIncrementReferenceCounts(
        MaidManager::MetadataKey(maid_name),
        ActionMaidManagerIncrementReferenceCounts(data_names), routing_.kNodeId()));
  }
}

void MaidManagerService::HandleDecrementReferenceCounts(const MaidName& maid_name,
                                      const nfs_vault::DataNames& data_names) {
  if (CheckDataNamesExist(maid_name, data_names)) {
    DoSync(MaidManager::UnresolvedDecrementReferenceCounts(
        MaidManager::MetadataKey(maid_name),
        ActionMaidManagerDecrementReferenceCounts(data_names), routing_.kNodeId()));
  }
}

bool MaidManagerService::CheckDataNamesExist(const MaidName& maid_name,
                                             const nfs_vault::DataNames& data_names) {
  try {
    for (const auto& data_name : data_names.data_names_) {
      MaidManager::Key key(MaidManager::GroupName(maid_name), data_name.raw_name, data_name.type);
      group_db_.GetValue(key);
    }
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
     return false;
  }
  return true;
}

void MaidManagerService::HandleSyncedIncrementReferenceCounts(
    std::unique_ptr<MaidManager::UnresolvedIncrementReferenceCounts>&&
        synced_action_increment_reference_counts) {
  MaidManager::MetadataKey metadata_key(synced_action_increment_reference_counts->key);
  for (const auto& data_name :
           synced_action_increment_reference_counts->action.kDataNames.data_names_) {
    MaidManager::Key key(MaidManager::GroupName(metadata_key.group_name()), data_name.raw_name,
                         ImmutableData::Tag::kValue);
    group_db_.Commit(key, ActionMaidManagerIncrementReferenceCount());
  }
}

void MaidManagerService::HandleSyncedDecrementReferenceCounts(
    std::unique_ptr<MaidManager::UnresolvedDecrementReferenceCounts>&&
        synced_action_decrement_reference_counts) {
  MaidManager::MetadataKey metadata_key(synced_action_decrement_reference_counts->key);
  for (const auto& data_name :
           synced_action_decrement_reference_counts->action.kDataNames.data_names_) {
    MaidManager::Key key(MaidManager::GroupName(metadata_key.group_name()), data_name.raw_name,
                         ImmutableData::Tag::kValue);
    group_db_.Commit(key, ActionMaidManagerDecrementReferenceCount());
  }
}

template <>
void MaidManagerService::HandleMessage(
    const AccountTransferFromMaidManagerToMaidManager& message,
    const typename AccountTransferFromMaidManagerToMaidManager::Sender& sender,
    const typename AccountTransferFromMaidManagerToMaidManager::Receiver& /*receiver*/) {
  MaidManager::UnresolvedAccountTransfer unresolved_account_transfer(message.contents->data);
  LOG(kInfo) << "MaidManager received account " << DebugId(sender.group_id)
             << " from " << DebugId(sender.sender_id);
  auto resolved_action(account_transfer_.AddUnresolvedAction(
      unresolved_account_transfer, sender,
      AccountTransfer<MaidManager::UnresolvedAccountTransfer>::AddRequestChecker(
          routing::Parameters::group_size / 2)));
  if (resolved_action) {
    LOG(kInfo) << "AccountTransferFromMaidManagerToMaidManager handle account transfer";
    this->HandleAccountTransfer(std::move(resolved_action));
  }
}

void MaidManagerService::HandleAccountTransfer(
    std::unique_ptr<MaidManager::UnresolvedAccountTransfer>&& resolved_action) {
  VLOG(nfs::Persona::kMaidManager, VisualiserAction::kGotAccountTransferred, resolved_action->key);
  GroupDb<MaidManager>::Contents content;
  content.group_name = resolved_action->key;
  for (auto& action : resolved_action->actions) {
    try {
      protobuf::MaidManagerKeyValuePair kv_msg;
      if (kv_msg.ParseFromString(action)) {
        LOG(kVerbose) << "HandleAccountTransfer handle key_value pair";
        MaidManager::Key key(kv_msg.key());
        LOG(kVerbose) << "HandleAccountTransfer key parsed";
        MaidManagerValue value(kv_msg.value());
        LOG(kVerbose) << "HandleAccountTransfer vaule parsed";
        content.kv_pairs.push_back(std::make_pair(key, std::move(value)));
      } else {
        LOG(kVerbose) << "HandleAccountTransfer handle metadata";
        MaidManagerMetadata meta_data(action);
        content.metadata = meta_data;
      }
    } catch(...) {
      LOG(kError) << "HandleAccountTransfer can't parse the action";
    }
  }
  LOG(kVerbose) << "MaidManagerService received account "<< content.Print();
  group_db_.HandleTransfer(content);
}

}  // namespace vault

}  // namespace maidsafe
