/* Copyright 2012 MaidSafe.net limited

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

#include "maidsafe/vault/pmid_account_holder/pmid_account_merge_policy.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account.pb.h"


namespace maidsafe {
namespace vault {

PmidAccountMergePolicy::PmidAccountMergePolicy(AccountDb* account_db)
    : unresolved_data_(),
      account_db_(account_db) {}

PmidAccountMergePolicy::PmidAccountMergePolicy(PmidAccountMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      account_db_(other.account_db_) {}

PmidAccountMergePolicy& PmidAccountMergePolicy::operator=(PmidAccountMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  return *this;
}

void PmidAccountMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
  auto serialised_db_value(GetFromDb(unresolved_entry.key.first));
  if (unresolved_entry.key.second == nfs::MessageAction::kPut &&
      !unresolved_entry.dont_add_to_db &&
      unresolved_entry.messages_contents.front().value) {
    MergePut(unresolved_entry.key.first, *unresolved_entry.messages_contents.front().value,
             serialised_db_value);
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
    MergeDelete(unresolved_entry.key.first, serialised_db_value);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void PmidAccountMergePolicy::MergePut(const DataNameVariant& data_name,
                                      UnresolvedEntry::Value size,
                                      const NonEmptyString& serialised_db_value) {
  if (!serialised_db_value.IsInitialised())
    account_db_->Put(std::make_pair(DbKey(data_name), SerialiseDbValue(Size(size))));
}

void PmidAccountMergePolicy::MergeDelete(const DataNameVariant& data_name,
                                         const NonEmptyString& serialised_db_value) {
  if (!serialised_db_value.IsInitialised()) {
    // No need to check in unresolved_data_, since the corresponding "Put" will already have been
    // marked as "dont_add_to_db".
    return;
  }
  account_db_->Delete(DbKey(data_name));
}

NonEmptyString PmidAccountMergePolicy::SerialiseDbValue(Size db_value) const {
  protobuf::PmidAccountDbValue proto_db_value;
  proto_db_value.set_size(db_value);
  return NonEmptyString(proto_db_value.SerializeAsString());
}

PmidAccountMergePolicy::Size PmidAccountMergePolicy::ParseDbValue(
    NonEmptyString serialised_db_value) const {
  protobuf::PmidAccountDbValue proto_db_value;
  if (!proto_db_value.ParseFromString(serialised_db_value.string()))
    ThrowError(CommonErrors::parsing_error);
  return Size(proto_db_value.size());
}

NonEmptyString PmidAccountMergePolicy::GetFromDb(const DataNameVariant& data_name) {
  NonEmptyString serialised_db_value;
  try {
    serialised_db_value = account_db_->Get(DbKey(data_name));
  }
  catch(const vault_error&) {}
  return serialised_db_value;
}


}  // namespace vault
}  // namespace maidsafe
