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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_INL_H_

#include <algorithm>

#include "maidsafe/common/on_scope_exit.h"


namespace maidsafe {

namespace vault {

template<typename Data>
MaidAccount::Status MaidAccount::PutData(const typename Data::name_type& name, int32_t cost) {
  if (total_claimed_available_size_by_pmids_ < total_put_data_ + cost)
    ThrowError(VaultErrors::not_enough_space);

  on_scope_exit strong_guarantee(on_scope_exit::RevertValue(recent_put_data_));
  auto result(DoPutData<Data>(name, cost));
  strong_guarantee.Release();
  return result;
}

template<typename Data>
void MaidAccount::DeleteData(const typename Data::name_type& name) {
  int32_t cost(0);
  auto itr(std::find_if(recent_put_data_.begin(),
                        recent_put_data_.end(),
                        [&name](const PutDataDetails& put_data_details) {
                            return DataNameVariant(name) == put_data_details.data_name_variant;
                        }));
  if (itr != recent_put_data_.end()) {
    cost = (*itr).cost;
    recent_put_data_.erase(itr);
  } else {
// TODO(Fraser) BEFORE_RELEASE Uncomment below.
//                                                          auto archive_future(archive_.Delete(name));
//                                                                        cost = archive_future.get();
  }
  total_put_data_ -= cost;
}

template<typename Data>
void MaidAccount::Adjust(const typename Data::name_type& name, int32_t new_cost) {
  on_scope_exit total_put_guarantee(on_scope_exit::RevertValue(total_put_data_)),
      recent_put_guarantee(on_scope_exit::RevertValue(recent_put_data_));
  DeleteData<Data>(name);
  if (total_claimed_available_size_by_pmids_ < total_put_data_ + new_cost)
    ThrowError(VaultErrors::not_enough_space);
  DoPutData<Data>(name, new_cost);
  total_put_guarantee.Release();
  recent_put_guarantee.Release();
}

template<typename Data>
MaidAccount::Status MaidAccount::DoPutData(const typename Data::name_type& name, int32_t cost) {
  recent_put_data_.emplace_back(name, cost);
  if (recent_put_data_.size() > detail::Parameters::max_recent_data_list_size) {
// TODO(Fraser) BEFORE_RELEASE Uncomment below.
//                                      auto archive_future(archive_.Put(recent_put_data_.front(), cost));
    recent_put_data_.pop_front();
    // Wait to check exception is not thrown.
    //archive_future.get();
  }
  total_put_data_ += cost;
  if (total_put_data_ > (total_claimed_available_size_by_pmids_ / 10) * 9)
    return Status::kLowSpace;
  else
    return Status::kOk;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_INL_H_
