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

#include "maidsafe/vault/pmid_account_holder/pmid_account_handler.h"

#include "boost/filesystem/operations.hpp"

#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {
namespace vault {

PmidAccountHandler::PmidAccountHandler(Db& db, const NodeId& this_node_id)
    : db_(db),
      kThisNodeId_(this_node_id),
      mutex_(),
      pmid_accounts_() {}

bool PmidAccountHandler::ApplyAccountTransfer(const PmidName& account_name, const NodeId &source_id,
    const PmidAccount::serialised_type& serialised_pmid_account_details) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<PmidAccount> account(new PmidAccount(account_name, db_, kThisNodeId_,
                                                       source_id,
                                                       serialised_pmid_account_details));
  if (!pmid_accounts_.insert(std::make_pair(account_name, std::move(account))).second)
    return false;
  else
    return pmid_accounts_.at(account_name)->ApplyAccountTransfer(source_id,
                                                                 serialised_pmid_account_details);
}

void PmidAccountHandler::AddAccount(std::unique_ptr<PmidAccount> account) {
  pmid_accounts_.insert(std::make_pair(account->name(), std::move(account)));
}

void PmidAccountHandler::DeleteAccount(const PmidName& account_name) {
  pmid_accounts_.erase(account_name);
}

PmidAccount::DataHolderStatus PmidAccountHandler::AccountStatus(
    const PmidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pmid_accounts_.at(account_name)->data_holder_status();
}

void PmidAccountHandler::SetDataHolderGoingDown(const PmidName& /*account_name*/) {
}

void PmidAccountHandler::SetDataHolderDown(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->SetDataHolderDown();
}

void PmidAccountHandler::SetDataHolderGoingUp(const PmidName& /*account_name*/) {
}

void PmidAccountHandler::SetDataHolderUp(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->SetDataHolderUp();
}

std::vector<PmidName> PmidAccountHandler::GetAccountNames() const {
  std::vector<PmidName> account_names;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& pmid_account : pmid_accounts_)
    account_names.push_back(pmid_account.first);
  return account_names;
}

PmidAccount::serialised_type PmidAccountHandler::GetSerialisedAccount(
    const PmidName& account_name) const {
  return pmid_accounts_.at(account_name)->Serialise();
}

NonEmptyString PmidAccountHandler::GetSyncData(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  return pmid_accounts_.at(account_name)->GetSyncData();
}

void PmidAccountHandler::ApplySyncData(const PmidName& account_name,
                                       const NonEmptyString& serialised_unresolved_entries) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->ApplySyncData(serialised_unresolved_entries);
}

void PmidAccountHandler::ReplaceNodeInSyncList(const PmidName& account_name,
                                               const NodeId& old_node,
                                               const NodeId& new_node) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->ReplaceNodeInSyncList(old_node, new_node);
}

void PmidAccountHandler::IncrementSyncAttempts(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->IncrementSyncAttempts();
}

}  // namespace vault
}  // namespace maidsafe
