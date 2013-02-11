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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_INL_H_

#include "maidsafe/common/on_scope_exit.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccount::PutData(const typename Data::name_type& name,
                          int32_t size,
                          int32_t replication_count) {
  // TODO(Fraser#5#): 2013-01-25 - BEFORE_RELEASE - Decide factor.
  if (2 * total_data_stored_by_pmids_ < total_put_data_ + size)
    ThrowError(VaultErrors::not_enough_space);

  on_scope_exit strong_guarantee(on_scope_exit::RevertValue(recent_put_data_));
  recent_put_data_.emplace_back(name, size, replication_count);
  if (recent_put_data_.size() > detail::Parameters::max_recent_data_list_size) {
//    archive_.Store(recent_put_data_.front());  // FIXME FRASER
    recent_put_data_.pop_front();
  }
  strong_guarantee.Release();
}

template<typename Data>
void MaidAccount::DeleteData(const typename Data::name_type& /*name*/) {
//  -- throw if data entry doesn't exist
}

template<typename Data>
void MaidAccount::UpdateReplicationCount(const typename Data::name_type& /*name*/,
                                         int32_t /*new_replication_count*/) {
}


//  void MaidAccount::PushDataElement(DataElement data_element) {
//    std::lock_guard<std::mutex> lock(mutex_);
//    data_elements_.push_back(data_element);
//  }

//  void MaidAccount::RemoveDataElement(Identity name) {
//    std::lock_guard<std::mutex> lock(mutex_);
//    for (auto itr = data_elements_.begin(); itr != data_elements_.end(); ++itr) {
//      if ((*itr).name() == name) {
//        data_elements_.erase(itr);
//        return;
//      }
//    }
//  }

//  void MaidAccount::UpdateDataElement(DataElement data_element) {
//    RemoveDataElement(data_element.name());
//    PushDataElement(data_element);
//  }

//  bool MaidAccount::HasDataElement(Identity name) {
//    std::lock_guard<std::mutex> lock(mutex_);
//    auto data_element_it = std::find_if(data_elements_.begin(), data_elements_.end(),
//                                        [&name] (const DataElement& data_element) {
//                                          return data_element.name() == name;
//                                        });
//    return (data_element_it != data_elements_.end());
//  }

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_INL_H_
