/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/version_manager/merge_policy.h"

#include "maidsafe/common/error.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/maid_manager/maid_account.pb.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

VersionManagerMergePolicy::VersionManagerMergePolicy(ManagerDb<VersionManager> *db)
    : unresolved_data_(),
      db_(db) {}

VersionManagerMergePolicy::VersionManagerMergePolicy(VersionManagerMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      db_(std::move(other.db_)) {}

VersionManagerMergePolicy& VersionManagerMergePolicy::operator=(VersionManagerMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  db_ = std::move(other.db_);
  return *this;
}

void VersionManagerMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
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
      VersionManagerVersions(*unresolved_entry.messages_contents.at(0).value->serialised_db_value));
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDeleteBranchUntilFork) {
    assert(unresolved_entry.messages_contents.at(0).value);
    MergeDeleteBranchUntilFork(unresolved_entry.key.first,
                               *unresolved_entry.messages_contents.at(0).value->version);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void VersionManagerMergePolicy::MergePut(const DbKey& key,
                                         const VersionManagerVersions::VersionName& new_value,
                                         const VersionManagerVersions::VersionName& old_value) {
  auto value(db_->Get(key));
  value.Put(old_value, new_value);
  db_->Put(std::make_pair(key, value));
}

void VersionManagerMergePolicy::MergeDeleteBranchUntilFork(
    const DbKey& key,
    const VersionManagerVersions::VersionName& tot) {
  auto value(db_->Get(key));
  value.DeleteBranchUntilFork(tot);
  db_->Put(std::make_pair(key, value));
}

void VersionManagerMergePolicy::MergeDelete(const DbKey& key) {
  db_->Delete(key);
}

void VersionManagerMergePolicy::MergeAccountTransfer(const DbKey& key,
                                                     const VersionManagerVersions& data_version) {
  db_->Put(std::make_pair(key, data_version));
}

}  // namespace vault

}  // namespace maidsafe
