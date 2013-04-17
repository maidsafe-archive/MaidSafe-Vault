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

#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace maidsafe {

namespace vault {

MaidAccountMergePolicy::MaidAccountMergePolicy(Db* db)
    : unresolved_data_(),
      db_(db) {}

MaidAccountMergePolicy::MaidAccountMergePolicy(MaidAccountMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      db_(std::move(other.db_)) {}

MaidAccountMergePolicy& MaidAccountMergePolicy::operator=(MaidAccountMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  db_ = std::move(other.db_);
  return *this;
}

void MaidAccountMergePolicy::Merge(const UnresolvedAction::Key& data_name_and_action,
                                   UnresolvedAction::Value cost) {
}

NonEmptyString MaidAccountMergePolicy::SerialiseDbValue(DbValue db_value) const {
}

MaidAccountMergePolicy::DbValue MaidAccountMergePolicy::ParseDbValue(
    NonEmptyString serialised_db_value) const {
}

}  // namespace vault

}  // namespace maidsafe
