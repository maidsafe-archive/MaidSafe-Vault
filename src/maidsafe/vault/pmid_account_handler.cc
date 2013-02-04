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

#include "maidsafe/vault/pmid_account_handler.h"

#include "boost/filesystem/operations.hpp"

namespace maidsafe {

namespace vault {

PmidAccountHandler::PmidAccountHandler(const boost::filesystem::path& vault_root_dir)
    : kPmidAccountsRoot_(vault_root_dir / "pmids"),
      mutex_(),
      pmid_accounts_(),
      archived_accounts_() {
  if (boost::filesystem::exists(kPmidAccountsRoot_)) {
    if (!boost::filesystem::is_directory(kPmidAccountsRoot_))
      ThrowError(CommonErrors::not_a_directory);
  } else {
    boost::filesystem::create_directories(kPmidAccountsRoot_);
  }
}

bool PmidAccountHandler::AddAccount(std::unique_ptr<PmidAccount> pmid_account) {
  return detail::AddAccount(mutex_, pmid_accounts_, std::move(pmid_account));
}

bool PmidAccountHandler::DeleteAccount(const PmidName& account_name) {
  return detail::DeleteAccount(mutex_, pmid_accounts_, account_name);
}

PmidAccount::DataHolderStatus PmidAccountHandler::AccountStatus(
    const PmidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  return (*itr)->data_holder_status();
}

void PmidAccountHandler::SetDataHolderGoingDown(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->SetDataHolderGoingDown();
}

void PmidAccountHandler::SetDataHolderDown(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->SetDataHolderDown();
}

void PmidAccountHandler::SetDataHolderGoingUp(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->SetDataHolderGoingUp();
}

void PmidAccountHandler::SetDataHolderUp(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->SetDataHolderUp();
}

std::vector<PmidName> PmidAccountHandler::GetAccountNames() const {
  std::vector<PmidName> account_names;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& pmid_account : pmid_accounts_)
    account_names.push_back(pmid_account->name());
  return account_names;
}

std::vector<PmidName> PmidAccountHandler::GetArchivedAccountNames() const {
  std::vector<PmidName> archived_names;
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& archived_account : archived_accounts_)
    archived_names.push_back(archived_account);
  return archived_names;
}

PmidAccount::serialised_type PmidAccountHandler::GetSerialisedAccount(
    const PmidName& account_name) const {
  return detail::GetSerialisedAccount(mutex_, pmid_accounts_, account_name);
}

std::vector<boost::filesystem::path> PmidAccountHandler::GetArchiveFileNames(
    const PmidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  return (*itr)->GetArchiveFileNames();
}

NonEmptyString PmidAccountHandler::GetArchiveFile(const PmidName& account_name,
                                                  const boost::filesystem::path& path) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  return (*itr)->GetArchiveFile(path);
}

void PmidAccountHandler::PutArchiveFile(const PmidName& account_name,
                                        const boost::filesystem::path& path,
                                        const NonEmptyString& content) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->PutArchiveFile(path, content);
}

void PmidAccountHandler::PruneArchivedAccounts(
    std::function<bool(const PmidName& account_name)> criteria) {
  std::lock_guard<std::mutex> lock(mutex_);
  archived_accounts_.erase(std::remove_if(archived_accounts_.begin(),
                                          archived_accounts_.end(),
                                          criteria),
                           archived_accounts_.end());
}

void PmidAccountHandler::MoveAccountToArchive(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(pmid_accounts_, account_name));
  if (itr != pmid_accounts_.end())
    pmid_accounts_.erase(itr);

  auto archive_itr(std::find(archived_accounts_.begin(), archived_accounts_.end(), account_name));
#ifdef NDEBUG
  if (archive_itr != archived_accounts_.end()) {
    LOG(kInfo) << "PMID account already in the archive list.";
    return;
  }
#else
  assert(archive_itr != archived_accounts_.end());
#endif
  archived_accounts_.push_back(account_name);
}

void PmidAccountHandler::BringAccountBackFromArchive(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto archive_itr(std::find(archived_accounts_.begin(), archived_accounts_.end(), account_name));
  if (archive_itr == archived_accounts_.end())
    ThrowError(VaultErrors::no_such_account);

  pmid_accounts_.push_back(std::unique_ptr<PmidAccount>(new PmidAccount(account_name,
                                                                        kPmidAccountsRoot_)));

  std::remove(archived_accounts_.begin(), archived_accounts_.end(), account_name);
}

void PmidAccountHandler::ArchiveRecentData(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::ConstFindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->ArchiveRecentData();
}

}  // namespace vault

}  // namespace maidsafe
