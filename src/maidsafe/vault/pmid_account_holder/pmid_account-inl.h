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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_INL_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_INL_H_

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void PmidAccount::PutData(const typename Data::name_type& name, int32_t size) {
  for (auto& record : recent_data_stored_) {
    if (detail::IsDataElement<Data>(name, record.data_name_variant))
      return;
  }

  DataElement data_element(GetDataNameVariant(Data::name_type::tag_value,
                                              Identity(name)),
                           size);
  recent_data_stored_.push_back(data_element);
  pmid_record_.stored_count++;
  pmid_record_.stored_total_size += size;
}

template<typename Data>
void PmidAccount::DeleteData(const typename Data::name_type& name) {
  for (auto itr(recent_data_stored_.begin()); itr != recent_data_stored_.end(); ++itr)
    if (detail::IsDataElement<Data>(name, itr->data_name_variant)) {
      pmid_record_.stored_count--;
      pmid_record_.stored_total_size -= itr->size;
      recent_data_stored_.erase(itr);
      archive_.Delete<Data>(name);
    }
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_INL_H_
