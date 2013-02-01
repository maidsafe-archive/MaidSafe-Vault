/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_INL_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_INL_H_


namespace maidsafe {

namespace vault {

template<typename Data>
void PmidAccount::PutData(const typename Data::name_type& name,
                          int32_t size,
                          int32_t replication_count) {
  auto itr(recent_data_stored_.find(name));
  if (itr == recent_data_stored_.end()) {
    recent_data_stored_.insert(std::make_pair(name, size));
    pmid_record_.stored_count++;
    pmid_record_.stored_total_size += size;
  }
}

template<typename Data>
bool PmidAccount::DeleteData(const typename Data::name_type& name) {
  auto itr(recent_data_stored_.find(name));
  if (itr != recent_data_stored_.end()) {
    pmid_record_.stored_count--;
    pmid_record_.stored_total_size -= itr->second;
    recent_data_stored_.erase(itr);
    try {
      auto delete_future(archive_.Delete<Data>(name));
      delete_future.get();
      return true;
    } catch(const std::exception& e) {
      return false;
    }
  }
  return false;
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_INL_H_
