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

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


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
  auto db_key = std::make_pair(unresolved_entry.key.data_name, unresolved_entry.key.originator);
  auto serialised_value(GetFromDb(db_key));

  if (unresolved_entry.key.action == nfs::MessageAction::kGetBranch) {
    assert(unresolved_entry.messages_contents.at(0).value->version);
    assert(unresolved_entry.messages_contents.at(0).value->new_version);
    MergePut(db_key,
             *unresolved_entry.messages_contents.at(0).value->version,
             *unresolved_entry.messages_contents.at(0).value->new_version);
  } else if (unresolved_entry.key.action == nfs::MessageAction::kDelete) {
    MergeDelete(db_key);
  } else if (unresolved_entry.key.action == nfs::MessageAction::kDeleteBranchUntilFork) {
    assert(unresolved_entry.messages_contents.at(0).value);
    MergeDeleteBranchUntilFork(db_key,
                               *unresolved_entry.messages_contents.at(0).value->version);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void StructuredDataMergePolicy::MergePut(const DbKey& key,
              const typename StructuredDataVersions::VersionName& new_value,
              const typename StructuredDataVersions::VersionName& old_value) {
  auto value(db_->Get(key));
  value.Put(old_value, new_value);
  db_->Put(std::make_pair(key, value);
}

void StructuredDataMergePolicy::MergeDeleteBranchUntilFork(const DbKey& key,
                                const typename StructuredDataVersions::VersionName& tot) {
  auto value(db_->Get(key));
  value.DeleteBranchUntilFork(tot);
  db_->Put(std::make_pair(key, value);
}

void StructuredDataMergePolicy::MergeDelete(const DbKey& key) {
  db_->Delete(key);
}

}  // namespace vault

}  // namespace maidsafe
