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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_INL_H_

#include "maidsafe/passport/types.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data>
struct AccountRequired : std::true_type {};

template<>
struct AccountRequired<passport::Anmaid> : std::false_type {};

template<>
struct AccountRequired<passport::Maid> : std::false_type {};

}  // namespace detail


template<typename Data>
void MaidAccountHandler::CreateAccount(const MaidName& account_name, AllowedAccountCreationType) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, db_, kThisNodeId_));
  // if account exists, this is a no-op (allow Maid and Anmaid to be stored several times)
  maid_accounts_.insert(std::move(std::make_pair(account_name, std::move(account))));
}

template<typename Data>
void MaidAccountHandler::DeleteData(const MaidName& account_name,
                                    const typename Data::name_type& data_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->DeleteData<Data>(data_name);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_INL_H_
