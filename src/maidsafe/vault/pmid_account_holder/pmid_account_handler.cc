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

PmidAccountHandler::PmidAccountHandler(const fs::path& vault_root_dir)
    : kPmidAccountsRoot_(vault_root_dir / "pmids"),
      mutex_(),
      pmid_accounts_(),
      archived_accounts_() {
  detail::InitialiseDirectory(kPmidAccountsRoot_);
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
  auto itr(detail::FindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  return (*itr)->data_holder_status();
}


void PmidAccountHandler::SetDataHolderDown(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(pmid_accounts_, account_name));
  if (itr == pmid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->SetDataHolderDown();
}


void PmidAccountHandler::SetDataHolderUp(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(pmid_accounts_, account_name));
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


PmidAccount::serialised_type PmidAccountHandler::GetSerialisedAccount(
    const PmidName& account_name) const {
  return detail::GetSerialisedAccount(mutex_, pmid_accounts_, account_name);
}



}  // namespace vault

}  // namespace maidsafe
