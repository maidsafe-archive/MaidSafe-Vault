/***************************************************************************************************
 *  Copyright 2012 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include "maidsafe/vault/maid_account_holder/maid_account_merge_policy.h"


namespace maidsafe {

namespace vault {

MaidAccountMergePolicy::MaidAccountMergePolicy(DbWrapper* db_wrapper)
    : unresolved_data_(),
      db_(db_wrapper) {}

MaidAccountMergePolicy::MaidAccountMergePolicy(MaidAccountMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      db_(std::move(other.db_)) {}

MaidAccountMergePolicy& MaidAccountMergePolicy::operator=(MaidAccountMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  db_ = std::move(other.db_);
  return *this;
}

void MaidAccountMergePolicy::MergePut(const DataNameVariant& key, const NonEmptyString& value) {
}

void MaidAccountMergePolicy::MergeDelete(const DataNameVariant& key, const NonEmptyString& value) {
}

}  // namespace vault

}  // namespace maidsafe
