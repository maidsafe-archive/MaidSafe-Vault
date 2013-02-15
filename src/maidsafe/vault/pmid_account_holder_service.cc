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

#include "maidsafe/vault/pmid_account_holder_service.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/pmid_account_pb.h"


namespace maidsafe {

namespace vault {

typedef std::vector<boost::filesystem::path> PathVector;

PmidAccountHolderService::PmidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   const boost::filesystem::path& vault_root_dir)
  : routing_(routing),
    public_key_getter_(public_key_getter),
    accumulator_(),
    pmid_account_handler_(vault_root_dir),
    nfs_(routing, pmid) {}


void PmidAccountHolderService::TriggerSync(
    /*const std::vector<routing::NodeInfo>& new_close_nodes*/) {
  // Operations to be done when we this call is received
  CheckAccounts();
}

void PmidAccountHolderService::CheckAccounts() {
  // Non-archived
  std::vector<PmidName> accounts_held(pmid_account_handler_.GetAccountNames());
  for (auto it(accounts_held.begin()); it != accounts_held.end(); ++it) {
    bool is_connected(routing_.IsConnectedVault(NodeId(*it)));
    PmidAccount::DataHolderStatus account_status(pmid_account_handler_.AccountStatus(*it));
    if (AssessRange(*it, account_status, is_connected))
      it = accounts_held.erase(it);
  }

  // Archived
  pmid_account_handler_.PruneArchivedAccounts(
      [this] (const PmidName& pmid_name) {
        return routing::GroupRangeStatus::kOutwithRange ==
               routing_.IsNodeIdInGroupRange(NodeId(pmid_name));
      });
}

bool PmidAccountHolderService::AssessRange(const PmidName& account_name,
                                           PmidAccount::DataHolderStatus account_status,
                                           bool is_connected) {
  int temp_int(0);
  switch (temp_int/*routing_.IsNodeIdInGroupRange(NodeId(account_name))*/) {
    // TODO(Team): Change to check the range
    case 0 /*routing::kOutwithRange*/:
        pmid_account_handler_.MoveAccountToArchive(account_name);
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

void PmidAccountHolderService::ValidateDataMessage(const nfs::DataMessage& data_message) const {
  if (!data_message.HasDataHolder()) {
    LOG(kError) << "No target ID, can't forward the message.";
    ThrowError(VaultErrors::permission_denied);
  }

  if (routing_.EstimateInGroup(data_message.source().node_id, NodeId(data_message.data().name))) {
    LOG(kError) << "Message doesn't seem to come from the right group.";
    ThrowError(VaultErrors::permission_denied);
  }
}

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

void PmidAccountHolderService::InformAboutDataHolder(const PmidName& pmid_name, bool node_up) {
  // TODO(Team): Decide on a better strategy instead of sleep
  Sleep(boost::posix_time::minutes(3));
  PathVector names(pmid_account_handler_.GetArchiveFileNames(pmid_name));
  for (auto ritr(names.rbegin()); ritr != names.rend(); ++ritr) {
    if (StatusHasReverted(pmid_name, node_up)) {
      RevertMessages(pmid_name, names.rbegin(), ritr, !node_up);
      return;
    }

    std::set<PmidName> metadata_manager_ids(GetDataNamesInFile(pmid_name, *ritr));
    SendMessages(pmid_name, metadata_manager_ids, node_up);
  }
}

std::set<PmidName> PmidAccountHolderService::GetDataNamesInFile(
    const PmidName& pmid_name,
    const boost::filesystem::path& path) const {
  NonEmptyString file_content(pmid_account_handler_.GetArchiveFile(pmid_name, path));
  protobuf::ArchivedPmidData pmid_data;
  pmid_data.ParseFromString(file_content.string());
  std::set<PmidName> metadata_manager_ids;
  for (int n(0); n != pmid_data.data_stored_size(); ++n)
    metadata_manager_ids.insert(PmidName(Identity(pmid_data.data_stored(n).name())));
  return metadata_manager_ids;
}

bool PmidAccountHolderService::StatusHasReverted(const PmidName& pmid_name, bool node_up) const {
  PmidAccount::DataHolderStatus status(pmid_account_handler_.AccountStatus(pmid_name));
  if (status == PmidAccount::DataHolderStatus::kGoingDown && node_up)
    return true;
  else if (status == PmidAccount::DataHolderStatus::kGoingUp && !node_up)
    return true;
  else
    return false;
}

void PmidAccountHolderService::RevertMessages(const PmidName& pmid_name,
                                              const PathVector::reverse_iterator& begin,
                                              PathVector::reverse_iterator& current,
                                              bool node_up) {
  while (current != begin) {
    std::set<PmidName> metadata_manager_ids(GetDataNamesInFile(pmid_name, *current));
    SendMessages(pmid_name, metadata_manager_ids, node_up);
    --current;
  }

  node_up ?  pmid_account_handler_.SetDataHolderUp(pmid_name) :
             pmid_account_handler_.SetDataHolderDown(pmid_name);
}

void PmidAccountHolderService::SendMessages(const PmidName& pmid_name,
                                            const std::set<PmidName>& metadata_manager_ids,
                                            bool node_up) {
  for (const PmidName& metadata_manager_id : metadata_manager_ids) {
    //TODO(dirvine) impliment
        //    nfs_.DataHolderStatusChanged(NodeId(metadata_manager_id), NodeId(pmid_name), node_up);
  }
}

}  // namespace vault

}  // namespace maidsafe
