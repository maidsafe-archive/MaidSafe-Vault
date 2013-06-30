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

#include "maidsafe/vault/maid_manager/merge_policy.h"

#include <set>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"


namespace maidsafe {

namespace vault {

MaidManagerMergePolicy::MaidManagerMergePolicy(AccountDb* account_db)
    : unresolved_data_(),
      account_db_(account_db) {}

MaidManagerMergePolicy::MaidManagerMergePolicy(MaidManagerMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      account_db_(std::move(other.account_db_)) {}

MaidManagerMergePolicy& MaidManagerMergePolicy::operator=(MaidManagerMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  account_db_ = std::move(other.account_db_);
  return *this;
}

void MaidManagerMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
  auto serialised_db_value(GetFromDb(unresolved_entry.key.first));
  if (unresolved_entry.key.second == nfs::MessageAction::kPut &&
      !unresolved_entry.dont_add_to_db) {
    MergePut(unresolved_entry.key.first, MergedCost(unresolved_entry), serialised_db_value);
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
    MergeDelete(unresolved_entry.key.first, serialised_db_value);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

MaidManagerMergePolicy::UnresolvedEntry::Value MaidManagerMergePolicy::MergedCost(
    const UnresolvedEntry& unresolved_entry) const {
  assert(unresolved_entry.key.second == nfs::MessageAction::kPut &&
         !unresolved_entry.dont_add_to_db);
  std::map<UnresolvedEntry::Value, size_t> all_costs;
  auto most_frequent_itr(std::end(unresolved_entry.messages_contents));
  size_t most_frequent(0);
  for (auto itr(std::begin(unresolved_entry.messages_contents));
       itr != std::end(unresolved_entry.messages_contents); ++itr) {
    if ((*itr).value) {
      size_t this_value_count(++all_costs[*(*itr).value]);
      if (this_value_count > most_frequent) {
        most_frequent = this_value_count;
        most_frequent_itr = itr;
      }
    }
  }

  if (all_costs.empty())
    ThrowError(CommonErrors::unknown);
  // This will always return here if all_costs.size() == 1, or if == 2 and both costs are the same.
  if (most_frequent > all_costs.size() / 2)
    return *(*most_frequent_itr).value;
  // Strip the first and last costs if they only have a count of 1.
  if (all_costs.size() > 2U) {
    if ((*std::begin(all_costs)).second == 1)
      all_costs.erase(std::begin(all_costs));
    if ((*(--std::end(all_costs))).second == 1)
      all_costs.erase(--std::end(all_costs));
  }

  UnresolvedEntry::Value total_cost(0);
  int count(0);
  for (const auto& cost : all_costs) {
    total_cost += static_cast<int32_t>(cost.first * cost.second);
    count += static_cast<int32_t>(cost.second);
  }

  return total_cost / count;
}

void MaidManagerMergePolicy::MergePut(const DataNameVariant& data_name,
                                      UnresolvedEntry::Value cost,
                                      const NonEmptyString& serialised_db_value) {
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    uint64_t current_total_size(current_values.first.data * current_values.second.data);
    ++current_values.second.data;
    current_values.first.data =
        static_cast<int32_t>((current_total_size + cost) / current_values.second.data);
    account_db_->Put(std::make_pair(DbKey(data_name), SerialiseDbValue(current_values)));
  } else {
    DbValue db_value(std::make_pair(AverageCost(cost), Count(1)));
    account_db_->Put(std::make_pair(DbKey(data_name), SerialiseDbValue(db_value)));
  }
}

void MaidManagerMergePolicy::MergeDelete(const DataNameVariant& data_name,
                                         const NonEmptyString& serialised_db_value) {
  if (!serialised_db_value.IsInitialised()) {
    // No need to check in unresolved_data_, since the corresponding "Put" will already have been
    // marked as "dont_add_to_db".
    return;
  }

  auto current_values(ParseDbValue(serialised_db_value));
  assert(current_values.second.data > 0);
  if (current_values.second.data == 1) {
    account_db_->Delete(DbKey(data_name));
  } else {
    --current_values.second.data;
    account_db_->Put(std::make_pair(DbKey(data_name), SerialiseDbValue(current_values)));
  }
}

NonEmptyString MaidManagerMergePolicy::SerialiseDbValue(DbValue db_value) const {
  protobuf::MaidManagerDbValue proto_db_value;
  proto_db_value.set_average_cost(db_value.first.data);
  proto_db_value.set_count(db_value.second.data);
  return NonEmptyString(proto_db_value.SerializeAsString());
}

MaidManagerMergePolicy::DbValue MaidManagerMergePolicy::ParseDbValue(
    NonEmptyString serialised_db_value) const {
  protobuf::MaidManagerDbValue proto_db_value;
  if (!proto_db_value.ParseFromString(serialised_db_value.string()))
    ThrowError(CommonErrors::parsing_error);
  return std::make_pair(AverageCost(proto_db_value.average_cost()), Count(proto_db_value.count()));
}

NonEmptyString MaidManagerMergePolicy::GetFromDb(const DataNameVariant& data_name) {
  try {
    return account_db_->Get(DbKey(data_name));
  }
  catch(const maidsafe_error&) {}
  return NonEmptyString();
}

}  // namespace vault

}  // namespace maidsafe
