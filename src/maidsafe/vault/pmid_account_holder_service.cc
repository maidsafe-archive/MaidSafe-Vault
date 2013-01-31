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

PmidAccountHolderService::PmidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   const boost::filesystem::path& vault_root_dir)
  : routing_(routing),
    public_key_getter_(public_key_getter),
    accumulator_(),
    pmid_account_handler_(vault_root_dir),
    nfs_(routing) {}


void PmidAccountHolderService::HandleSynchronise(
    const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
  std::vector<PmidName> accounts_held(pmid_account_handler_.GetAccountNames());
  for (auto it(accounts_held.begin()); it != accounts_held.end(); ++it) {
    if (!routing_.IsNodeIdInGroupRange(NodeId(*it))) {
      pmid_account_handler_.DeleteAccount(*it);
      it = accounts_held.erase(it);
    } else {
      ++it;
    }
  }

  for (auto& account : accounts_held) {
    bool is_connected(routing_.IsConnectedVault(NodeId(account)));
    PmidAccount::Status account_status(pmid_account_handler_.AccountStatus());
    if (account_status == PmidAccount::Status::kNodeUp && !is_connected) {
      InformOfDataHolderDown(account);
    } else if (account_status == PmidAccount::Status::kNodeDown && is_connected) {
      InformOfDataHolderUp(account);
    }
  }
}

void PmidAccountHolderService::ValidateDataMessage(const nfs::DataMessage& data_message) const {
  if (!data_message.HasTargetId()) {
    LOG(kError) << "No target ID, can't forward the message.";
    ThrowError(VaultErrors::permission_denied);
  }

  if (routing_.EstimateInGroup(data_message.this_persona().node_id,
                               NodeId(data_message.data().name))) {
    LOG(kError) << "Message doesn't seem to come from the right group.";
    ThrowError(VaultErrors::permission_denied);
  }
}

void PmidAccountHolderService::InformOfDataHolderDown(const PmidName& pmid_name) {
  InformAboutDataHolder(pmid_name, false);
}

void PmidAccountHolderService::InformOfDataHolderUp(const PmidName& pmid_name) {
  InformAboutDataHolder(pmid_name, true);
}

void PmidAccountHolderService::InformAboutDataHolder(const PmidName& pmid_name, bool node_up) {
  std::vector<PmidName> metadata_manager_ids(GetDataNamesInAccount(pmid_name));
  for (PmidName& metadata_manager_id : metadata_manager_ids)
    nfs_.DataHolderStatusChanged(metadata_manager_id, pmid_name);
}

std::vector<PmidName> PmidAccountHolderService::GetDataNamesInAccount(
    const PmidName& pmid_name) const {
  // TODO(Team): This function could be simplified if the account handler could give all
  //             data names for a particular account
  NonEmptyString serialised_account(pmid_account_handler_.GetSerialisedAccount(pmid_name));
  protobuf::PmidAccount account;
  account.ParseFromString(serialised_account.string());

  std::vector<PmidName> metadata_manager_ids;
  for (int n(0); n != account.recent_data_stored_size(); ++n)
    metadata_manager_ids.push_back(PmidName(Identity(account.recent_data_stored(n).name())));
  return metadata_manager_ids;
}

void PmidAccountHolderService::ProcessNodeDown(const PmidName& pmid_name) {
  Sleep(boost::posix_time::minutes(3));

}


}  // namespace vault

}  // namespace maidsafe
