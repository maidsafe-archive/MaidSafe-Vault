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

#include "maidsafe/vault/maid_manager/handler.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {

namespace vault {

/*
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
                                          const PmidManagerMetadata& pmid_record) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->UpdatePmidTotals(pmid_record);
}

void MaidAccountHandler::AddLocalUnresolvedAction(
    const MaidName& account_name,
    const MaidManagerUnresolvedAction& unresolved_action) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->AddLocalUnresolvedAction(unresolved_action);
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
                                       const NonEmptyString& serialised_unresolved_actions) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->ApplySyncData(serialised_unresolved_actions);
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
*/

}  // namespace vault

}  // namespace maidsafe
