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


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

const int PmidAccountHolderService::kPutRequestsRequired_(3);
const int PmidAccountHolderService::kDeleteRequestsRequired_(3);

PmidAccountHolderService::PmidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing)
    : routing_(routing),
      accumulator_mutex_(),
      accumulator_(),
      pmid_account_handler_(),
      nfs_(routing, pmid) {}


void PmidAccountHolderService::HandleGenericMessage(const nfs::Message& /*generic_message*/,
                                                    const routing::ReplyFunctor& /*reply_functor*/) {

}

void PmidAccountHolderService::HandleChurnEvent(routing::MatrixChange /*matrix_change*/) {
//    /*const std::vector<routing::NodeInfo>& new_close_nodes*/) {
//  // Operations to be done when we this call is received
//  CheckAccounts();
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

void PmidAccountHolderService::ValidateSender(const nfs::Message& data_message) const {
  if (!data_message.HasDataHolder())
    ThrowError(VaultErrors::permission_denied);

  if (!routing_.IsConnectedVault(NodeId(data_message.data_holder()->string())))
    ThrowError(VaultErrors::permission_denied);

  if (routing_.EstimateInGroup(data_message.source().node_id,
                               NodeId(data_message.data().name)))
    ThrowError(VaultErrors::permission_denied);

  if (data_message.source().persona != nfs::Persona::kMetadataManager ||
      data_message.destination_persona() != nfs::Persona::kPmidAccountHolder)
    ThrowError(CommonErrors::invalid_parameter);
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
