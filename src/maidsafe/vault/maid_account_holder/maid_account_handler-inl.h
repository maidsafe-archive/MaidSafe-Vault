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


namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data>
struct AccountRequired : true_type {};

template<>
struct AccountRequired<passport::Anmaid> : false_type {};

template<>
struct AccountRequired<passport::Maid> : false_type {};

}  // namespace detail


template<typename Data>
MaidAccount::Status MaidAccountHandler::PutData(const MaidName& account_name,
                                                const typename Data::name_type& data_name,
                                                int32_t cost,
                                                RequireAccount) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(maid_accounts_, account_name));
  if (itr == maid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  return (*itr)->PutData<Data>(data_name, cost);
}

template<typename Data>
MaidAccount::Status MaidAccountHandler::PutData(const MaidName& account_name,
                                                const typename Data::name_type& data_name,
                                                int32_t cost,
                                                RequireNoAccount) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(maid_accounts_, account_name));
  if (itr != maid_accounts_.end())
    ThrowError(VaultErrors::operation_not_supported);
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, kMaidAccountsRoot_));
  bool success(AddAccount(std::move(account)));
  assert(success);
  static_cast<void>(success);
  return (*itr)->PutData<Data>(data_name, cost);
}

template<typename Data>
void MaidAccountHandler::DeleteData(const MaidName& account_name,
                                    const typename Data::name_type& data_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(maid_accounts_, account_name));
  if (itr == maid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->DeleteData<Data>(data_name);
}

template<typename Data>
void MaidAccountHandler::Adjust(const MaidName& account_name,
                                const typename Data::name_type& data_name,
                                int32_t new_cost) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(detail::FindAccount(maid_accounts_, account_name));
  if (itr == maid_accounts_.end())
    ThrowError(VaultErrors::no_such_account);
  (*itr)->Adjust<Data>(data_name, new_cost);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_INL_H_
