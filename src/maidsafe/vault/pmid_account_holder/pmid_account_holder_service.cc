/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/pmid_account_holder/pmid_account_holder_service.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account.pb.h"
#include "maidsafe/vault/sync.pb.h"

namespace fs = boost::filesystem;

namespace maidsafe {
namespace vault {

const int PmidAccountHolderService::kPutRepliesSuccessesRequired_(1);
const int PmidAccountHolderService::kDeleteRequestsRequired_(3);

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidAccountHolder;
}

}  // unnamed namespace
PmidAccountHolderService::PmidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   Db& db)
    : routing_(routing),
      accumulator_mutex_(),
      accumulator_(),
      pmid_account_handler_(db, routing.kNodeId()),
      nfs_(routing, pmid) {}

void PmidAccountHolderService::HandleMessage(const nfs::Message& message,
                                             const routing::ReplyFunctor& reply_functor) {
  ValidateGenericSender(message);
  nfs::Reply reply(CommonErrors::success);
  nfs::MessageAction action(message.data().action);
  switch (action) {
    case nfs::MessageAction::kSynchronise:
      return HandleSync(message);
    case nfs::MessageAction::kAccountTransfer:
      return HandleAccountTransfer(message);
    case nfs::MessageAction::kNodeDown:
      return;
    case nfs::MessageAction::kNodeUp:
      return;
    case nfs::MessageAction::kGetPmidTotals:
      return HandleGetPmidTotals(message, reply_functor);
    default:
      LOG(kError) << "Unhandled Post action type";
  }

  reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
  reply_functor(reply.Serialise()->string());
}

void PmidAccountHolderService::HandleGetPmidTotals(const Message& message,
                                                   const ReplyFunctor& reply_functor) {
  try {
    PmidRecord pmid_record(pmid_account_handler_.GetPmidRecord(PmidName(message.data().name)));
    if (!pmid_record.pmid_name.data.string().empty())
      Reply reply(CommonErrors::success, pmid_record.Serialise());
      // send it...
      // nfs_.
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired_);
  }
}

void PmidAccountHolderService::HandleChurnEvent(routing::MatrixChange matrix_change) {
//  CheckAccounts();
  auto account_names(pmid_account_handler_.GetAccountNames());
  auto itr(std::begin(account_names));
  while (itr != std::end(account_names)) {
    auto check_holders_result(CheckHolders(matrix_change, routing_.kNodeId(),
                                           NodeId((*itr)->string())));
    // Delete accounts for which this node is no longer responsible.
    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      pmid_account_handler_.DeleteAccount(*itr);
      itr = account_names.erase(itr);
      continue;
    }

    // Replace old_node(s) in sync object and send AccountTransfer to new node(s).
    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
      pmid_account_handler_.ReplaceNodeInSyncList(*itr, check_holders_result.old_holders[i],
                                                  check_holders_result.new_holders[i]);
      TransferAccount(*itr, check_holders_result.new_holders[i]);
    }

    ++itr;
  }
}

void PmidAccountHolderService::CheckAccounts() {
//  // Non-archived
//  std::vector<PmidName> accounts_held(pmid_account_handler_.GetAccountNames());
//  for (auto it(accounts_held.begin()); it != accounts_held.end(); ++it) {
//    bool is_connected(routing_.IsConnectedVault(NodeId(*it)));
//    PmidAccount::DataHolderStatus account_status(pmid_account_handler_.AccountStatus(*it));
//    if (AssessRange(*it, account_status, is_connected))
//      it = accounts_held.erase(it);
//  }

//  // Archived
//  pmid_account_handler_.PruneArchivedAccounts(
//      [this] (const PmidName& pmid_name) {
//        return routing::GroupRangeStatus::kOutwithRange ==
//               routing_.IsNodeIdInGroupRange(NodeId(pmid_name));
//      });
}

bool PmidAccountHolderService::AssessRange(const PmidName& account_name,
                                           PmidAccount::DataHolderStatus account_status,
                                           bool is_connected) {
  int temp_int(0);
  switch (temp_int/*routing_.IsNodeIdInGroupRange(NodeId(account_name))*/) {
    // TODO(Team): Change to check the range
    case 0 /*routing::kOutwithRange*/:
//        pmid_account_handler_.MoveAccountToArchive(account_name);
        return true;
    case 1 /*routing::kInProximalRange*/:
        // serialise the memory deque and put to file
        return false;
    case 2 /*routing::kInRange*/:
        if (account_status == PmidAccount::DataHolderStatus::kUp && !is_connected) {
          InformOfDataHolderDown(account_name);
        } else if (account_status == PmidAccount::DataHolderStatus::kDown && is_connected) {
          InformOfDataHolderUp(account_name);
        }
        return false;
    default: return false;
  }
}

void PmidAccountHolderService::ValidateDataSender(const nfs::Message& message) const {
  if (!message.HasDataHolder()
      || !routing_.IsConnectedVault(NodeId(message.data_holder()->string()))
      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
    ThrowError(VaultErrors::permission_denied);

  if (!FromMetadataManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void PmidAccountHolderService::ValidateGenericSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedVault(message.source().node_id)
      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
    ThrowError(VaultErrors::permission_denied);

  if (!FromMetadataManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

// =============== Put/Delete data ================================================================

void PmidAccountHolderService::SendReplyAndAddToAccumulator(
    const nfs::Message& message,
    const routing::ReplyFunctor& reply_functor,
    const nfs::Reply& reply) {
  reply_functor(reply.Serialise()->string());
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(message, reply.error());
}


// =============== Sync ===========================================================================

void PmidAccountHolderService::Sync(const PmidName& account_name) {
  auto serialised_sync_data(pmid_account_handler_.GetSyncData(account_name));
  if (!serialised_sync_data.IsInitialised())  // Nothing to sync
    return;

  protobuf::Sync proto_sync;
  proto_sync.set_account_name(account_name->string());
  proto_sync.set_serialised_unresolved_entries(serialised_sync_data.string());

  nfs_.Sync(account_name, NonEmptyString(proto_sync.SerializeAsString()));
  // TODO(Team): 2013-05-07 - Check this is correct place to increment sync attempt counter.
  pmid_account_handler_.IncrementSyncAttempts(account_name);
}

void PmidAccountHolderService::HandleSync(const nfs::Message& message) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.data().content.string())) {
    LOG(kError) << "Error parsing Synchronise message.";
    return;
  }
  pmid_account_handler_.ApplySyncData(PmidName(Identity(proto_sync.account_name())),
                                      NonEmptyString(proto_sync.serialised_unresolved_entries()));
}

// =============== Account transfer ===============================================================

void PmidAccountHolderService::TransferAccount(const PmidName& account_name,
                                               const NodeId& new_node) {
  protobuf::PmidAccount pmid_account;
  pmid_account.set_pmid_name(account_name.data.string());
  PmidAccount::serialised_type
    serialised_account_details(pmid_account_handler_.GetSerialisedAccount(account_name));
  pmid_account.set_serialised_account_details(serialised_account_details.data.string());
  nfs_.TransferAccount(new_node, NonEmptyString(pmid_account.SerializeAsString()));
}

void PmidAccountHolderService::HandleAccountTransfer(const nfs::Message& message) {
  protobuf::PmidAccount pmid_account;
  NodeId source_id(message.source().node_id);
  if (!pmid_account.ParseFromString(message.data().content.string()))
    return;

  PmidName account_name(Identity(pmid_account.pmid_name()));
  bool finished_all_transfers(
      pmid_account_handler_.ApplyAccountTransfer(account_name, source_id,
         PmidAccount::serialised_type(NonEmptyString(pmid_account.serialised_account_details()))));
  if (finished_all_transfers)
    return;    // TODO(Team) Implement whatever else is required here?
}

// =============== DataHolder =====================================================================

void PmidAccountHolderService::InformOfDataHolderDown(const PmidName& pmid_name) {
  pmid_account_handler_.SetDataHolderGoingDown(pmid_name);
  InformAboutDataHolder(pmid_name, false);
  pmid_account_handler_.SetDataHolderDown(pmid_name);
}

void PmidAccountHolderService::InformOfDataHolderUp(const PmidName& pmid_name) {
  pmid_account_handler_.SetDataHolderGoingUp(pmid_name);
  InformAboutDataHolder(pmid_name, true);
  pmid_account_handler_.SetDataHolderUp(pmid_name);
}

void PmidAccountHolderService::InformAboutDataHolder(const PmidName& /*pmid_name*/, bool /*node_up*/) {
  // TODO(Team): Decide on a better strategy instead of sleep
//  Sleep(boost::posix_time::minutes(3));
//  auto names(pmid_account_handler_.GetArchiveFileNames(pmid_name));
//  for (auto ritr(names.rbegin()); ritr != names.rend(); ++ritr) {
//    if (StatusHasReverted(pmid_name, node_up)) {
//      RevertMessages(pmid_name, names.rbegin(), ritr, !node_up);
//      return;
//    }

//    std::set<PmidName> metadata_manager_ids(GetDataNamesInFile(pmid_name, *ritr));
//    SendMessages(pmid_name, metadata_manager_ids, node_up);
//  }
}

void PmidAccountHolderService::RevertMessages(const PmidName& pmid_name,
                                              const std::vector<fs::path>::reverse_iterator& begin,
                                              std::vector<fs::path>::reverse_iterator& current,
                                              bool node_up) {
  while (current != begin) {
    std::set<PmidName> metadata_manager_ids(GetDataNamesInFile(pmid_name, *current));
    SendMessages(pmid_name, metadata_manager_ids, node_up);
    --current;
  }

  node_up ?  pmid_account_handler_.SetDataHolderUp(pmid_name) :
             pmid_account_handler_.SetDataHolderDown(pmid_name);
}

std::set<PmidName> PmidAccountHolderService::GetDataNamesInFile(
    const PmidName& /*pmid_name*/, const boost::filesystem::path& /*path*/) const {
  return std::set<PmidName>();
}

void PmidAccountHolderService::SendMessages(const PmidName& /*pmid_name*/,
                                            const std::set<PmidName>& /*metadata_manager_ids*/,
                                            bool /*node_up*/) {
//  for (const PmidName& metadata_manager_id : metadata_manager_ids) {
    //  TODO(dirvine) implement
        //    nfs_.DataHolderStatusChanged(NodeId(metadata_manager_id), NodeId(pmid_name), node_up);
//  }
}

}  // namespace vault
}  // namespace maidsafe
