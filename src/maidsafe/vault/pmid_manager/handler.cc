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

#include "maidsafe/vault/pmid_manager/handler.h"

#include "boost/filesystem/operations.hpp"

#include "maidsafe/vault/utils.h"


//namespace fs = boost::filesystem;

//namespace maidsafe {
//namespace vault {

//PmidAccountHandler::PmidAccountHandler(Db& db, const NodeId& this_node_id)
//    : db_(db),
//      kThisNodeId_(this_node_id),
//      mutex_(),
//      pmid_accounts_() {}

//void PmidAccountHandler::CreateAccount(const PmidName& account_name) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  std::vector<PmidName> account_names(GetAccountNames());
//  for (auto& pmid_account_name : account_names)
//    if (pmid_account_name == account_name)
//      return;
//  std::unique_ptr<PmidAccount> account(new PmidAccount(account_name, db_, kThisNodeId_));
//  pmid_accounts_.insert(std::move(std::make_pair(account_name, std::move(account))));
//}

//bool PmidAccountHandler::ApplyAccountTransfer(const PmidName& account_name, const NodeId &source_id,
//    const PmidAccount::serialised_type& serialised_pmid_account_details) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  std::unique_ptr<PmidAccount> account(new PmidAccount(account_name, db_, kThisNodeId_,
//                                                       source_id,
//                                                       serialised_pmid_account_details));
//  if (!pmid_accounts_.insert(std::make_pair(account_name, std::move(account))).second)
//    return false;
//  else
//    return pmid_accounts_.at(account_name)->ApplyAccountTransfer(source_id,
//                                                                 serialised_pmid_account_details);
//}

//void PmidAccountHandler::AddAccount(std::unique_ptr<PmidAccount> account) {
//  pmid_accounts_.insert(std::make_pair(account->name(), std::move(account)));
//}

//void PmidAccountHandler::DeleteAccount(const PmidName& account_name) {
//  pmid_accounts_.erase(account_name);
//}

//PmidAccount::PmidNodeStatus PmidAccountHandler::PmidNodeStatus(
//    const PmidName& account_name) const {
//  std::lock_guard<std::mutex> lock(mutex_);
//  return pmid_accounts_.at(account_name)->pmid_node_status();
//}

//void PmidAccountHandler::SetPmidNodeDown(const PmidName& account_name) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  pmid_accounts_.at(account_name)->SetPmidNodeDown();
//}

//void PmidAccountHandler::SetPmidNodeUp(const PmidName& account_name) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  pmid_accounts_.at(account_name)->SetPmidNodeUp();
//}

//void PmidAccountHandler::AddLocalUnresolvedEntry(const PmidName& account_name,
//                                                 const PmidManagerUnresolvedEntry& unresolved_entry) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  pmid_accounts_.at(account_name)->AddLocalUnresolvedEntry(unresolved_entry);
//}

//PmidManagerMetadata PmidAccountHandler::GetMetadata(const PmidName& account_name) {
//  auto it = std::find_if(std::begin(pmid_accounts_),
//                         std::end(pmid_accounts_),
//                         [&account_name](const AccountMap::value_type& pmid_account) {
//                            return account_name == pmid_account.second->name();
//                         });
//  if (it != std::end(pmid_accounts_))
//    return it->second->GetMetadata();
//  return PmidManagerMetadata();
//}

//std::vector<PmidName> PmidAccountHandler::GetAccountNames() const {
//  std::vector<PmidName> account_names;
//  std::lock_guard<std::mutex> lock(mutex_);
//  for (auto& pmid_account : pmid_accounts_)
//    account_names.push_back(pmid_account.first);
//  return account_names;
//}

//PmidAccount::serialised_type PmidAccountHandler::GetSerialisedAccount(
//    const PmidName& account_name, bool include_pmid_record) const {
//  return pmid_accounts_.at(account_name)->Serialise(include_pmid_record);
//}

//NonEmptyString PmidAccountHandler::GetSyncData(const PmidName& account_name) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  return pmid_accounts_.at(account_name)->GetSyncData();
//}

//void PmidAccountHandler::ApplySyncData(const PmidName& account_name,
//                                       const NonEmptyString& serialised_unresolved_entries) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  return pmid_accounts_.at(account_name)->ApplySyncData(serialised_unresolved_entries);
//}

//void PmidAccountHandler::ReplaceNodeInSyncList(const PmidName& account_name,
//                                               const NodeId& old_node,
//                                               const NodeId& new_node) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  pmid_accounts_.at(account_name)->ReplaceNodeInSyncList(old_node, new_node);
//}

//}  // namespace vault
//}  // namespace maidsafe
