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
