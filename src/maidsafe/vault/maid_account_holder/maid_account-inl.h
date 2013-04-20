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
  if (state_.total_claimed_available_size_by_pmids < state_.total_put_data + cost)
    ThrowError(VaultErrors::not_enough_space);

  return DoPutData(cost);
}

template<typename Data>
void MaidAccount::DeleteData(const typename Data::name_type& name) {
  state_.total_put_data -= sync_.AllowDelete<Data>(name);
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_INL_H_
