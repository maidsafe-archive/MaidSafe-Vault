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

PmidAccountHandler::PmidAccountHandler()
    : mutex_(),
      pmid_accounts_() {}

void PmidAccountHandler::AddAccount(std::unique_ptr<PmidAccount> account) {

  pmid_accounts_.insert(std::make_pair(account->name, std::move(account))))
}

void PmidAccountHandler::DeleteAccount(const PmidName& account_name) {
  pmid_accounts_.erase(pmid_accounts_.find(account_name));
}

PmidAccount::DataHolderStatus PmidAccountHandler::AccountStatus(
    const PmidName& account_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pmid_accounts_.at(account_name)->data_holder_status();
}

void PmidAccountHandler::SetDataHolderDown(const PmidName& account_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->SetDataHolderDown();
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

}  // namespace vault

}  // namespace maidsafe
