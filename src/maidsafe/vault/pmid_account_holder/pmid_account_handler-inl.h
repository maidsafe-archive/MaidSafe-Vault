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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_INL_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_INL_H_

#include "maidsafe/vault/utils.h"

namespace maidsafe {
namespace vault {

template<typename Data>
void PmidAccountHandler::Put(const PmidName& account_name,
                             const typename Data::name_type& /*data_name*/,
                             int32_t size) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->PutData(size);
}

template<typename Data>
void PmidAccountHandler::Delete(const PmidName& account_name,
                                const typename Data::name_type& data_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  pmid_accounts_.at(account_name)->DeleteData<Data>(data_name);
}

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_INL_H_
