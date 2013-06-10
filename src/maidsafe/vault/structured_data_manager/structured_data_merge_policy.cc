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

#include "maidsafe/vault/structured_data_manager/structured_data_merge_policy.h"

#include "maidsafe/common/error.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

StructuredDataMergePolicy::StructuredDataMergePolicy(ManagerDb<StructuredDataManager> *db)
    : unresolved_data_(),
      db_(db) {}

StructuredDataMergePolicy::StructuredDataMergePolicy(StructuredDataMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      db_(std::move(other.db_)) {}

StructuredDataMergePolicy& StructuredDataMergePolicy::operator=(StructuredDataMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  db_ = std::move(other.db_);
  return *this;
}

void StructuredDataMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
  if (unresolved_entry.key.second == nfs::MessageAction::kPut) {
    assert(unresolved_entry.messages_contents.at(0).value->version);
    assert(unresolved_entry.messages_contents.at(0).value->new_version);
    MergePut(unresolved_entry.key.first,
             *unresolved_entry.messages_contents.at(0).value->version,
             *unresolved_entry.messages_contents.at(0).value->new_version);
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
    MergeDelete(unresolved_entry.key.first);
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
      assert(unresolved_entry.messages_contents.at(0).value->serialised_db_value);
      MergeAccountTransfer(unresolved_entry.key.first,
      StructuredDataVersions(*unresolved_entry.messages_contents.at(0).value->serialised_db_value));
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDeleteBranchUntilFork) {
    assert(unresolved_entry.messages_contents.at(0).value);
    MergeDeleteBranchUntilFork(unresolved_entry.key.first,
                               *unresolved_entry.messages_contents.at(0).value->version);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void StructuredDataMergePolicy::MergePut(const DbKey& key,
                                         const StructuredDataVersions::VersionName& new_value,
                                         const StructuredDataVersions::VersionName& old_value) {
  auto value(db_->Get(key));
  value.Put(old_value, new_value);
  db_->Put(std::make_pair(key, value));
}

void StructuredDataMergePolicy::MergeDeleteBranchUntilFork(
    const DbKey& key,
    const StructuredDataVersions::VersionName& tot) {
  auto value(db_->Get(key));
  value.DeleteBranchUntilFork(tot);
  db_->Put(std::make_pair(key, value));
}

void StructuredDataMergePolicy::MergeDelete(const DbKey& key) {
  db_->Delete(key);
}

void StructuredDataMergePolicy::MergeAccountTransfer(const DbKey& key,
                                                     const StructuredDataVersions& data_version) {
  db_->Put(std::make_pair(key, data_version));
}

}  // namespace vault

}  // namespace maidsafe
