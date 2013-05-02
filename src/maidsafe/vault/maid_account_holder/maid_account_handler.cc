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

#include "maidsafe/vault/maid_account_holder/maid_account_handler.h"

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

MaidAccountHandler::MaidAccountHandler(Db& db, const NodeId& this_node_id)
    : db_(db),
      kThisNodeId_(this_node_id),
      mutex_(),
      maid_accounts_() {}

bool MaidAccountHandler::ApplyAccountTransfer(const MaidName& account_name, const NodeId &source_id,
    const MaidAccount::serialised_type& serialised_maid_account_details) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, db_, kThisNodeId_,
                                                       source_id,
                                                       serialised_maid_account_details));
  if (!maid_accounts_.insert(std::make_pair(account_name, std::move(account))).second)
    return false;
  else
    return maid_accounts_.at(account_name)->ApplyAccountTransfer(source_id,
                                                                 serialised_maid_account_details);
}

void MaidAccountHandler::DeleteAccount(const MaidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.erase(account_name);
}

void MaidAccountHandler::RegisterPmid(const MaidName& account_name,
                                      const nfs::PmidRegistration& pmid_registration) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->RegisterPmid(pmid_registration);
}

void MaidAccountHandler::UnregisterPmid(const MaidName& account_name, const PmidName& pmid_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->UnregisterPmid(pmid_name);
}

std::vector<PmidName> MaidAccountHandler::GetPmidNames(const MaidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return maid_accounts_.at(account_name)->GetPmidNames();
}

void MaidAccountHandler::UpdatePmidTotals(const MaidName& account_name,
                                          const PmidTotals& pmid_totals) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->UpdatePmidTotals(pmid_totals);
}

void MaidAccountHandler::AddLocalUnresolvedEntry(
    const MaidName& account_name,
    const MaidAccountUnresolvedEntry& unresolved_entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->AddLocalUnresolvedEntry(unresolved_entry);
}

std::vector<MaidName> MaidAccountHandler::GetAccountNames() const {
  std::vector<MaidName> account_names;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& maid_account : maid_accounts_)
    account_names.push_back(maid_account.first);
  return account_names;
}

MaidAccount::serialised_type MaidAccountHandler::GetSerialisedAccount(
    const MaidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return maid_accounts_.at(account_name)->Serialise();
}

NonEmptyString MaidAccountHandler::GetSyncData(const MaidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  return maid_accounts_.at(account_name)->GetSyncData();
}

void MaidAccountHandler::ApplySyncData(const MaidName& account_name,
                                       const NonEmptyString& serialised_unresolved_entries) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->ApplySyncData(serialised_unresolved_entries);
}

void MaidAccountHandler::ReplaceNodeInSyncList(const MaidName& account_name,
                                               const NodeId& old_node,
                                               const NodeId& new_node) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->ReplaceNodeInSyncList(old_node, new_node);
}

void MaidAccountHandler::IncrementSyncAttempts(const MaidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->IncrementSyncAttempts();
}

MaidAccount::Status MaidAccountHandler::AllowPut(const MaidName& account_name, int32_t cost) const {
  if (cost > 0) {
    std::lock_guard<std::mutex> lock(mutex_);
    return maid_accounts_.at(account_name)->AllowPut(cost);
  } else {
    return MaidAccount::Status::kOk;
  }
}

}  // namespace vault

}  // namespace maidsafe
