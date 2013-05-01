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

#include <set>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace maidsafe {

namespace vault {

MaidAccountMergePolicy::MaidAccountMergePolicy(AccountDb* account_db)
    : unresolved_data_(),
      account_db_(account_db) {}

MaidAccountMergePolicy::MaidAccountMergePolicy(MaidAccountMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      account_db_(std::move(other.account_db_)) {}

MaidAccountMergePolicy& MaidAccountMergePolicy::operator=(MaidAccountMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  account_db_ = std::move(other.account_db_);
  return *this;
}

void MaidAccountMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
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

MaidAccountMergePolicy::UnresolvedEntry::Value MaidAccountMergePolicy::MergedCost(
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
  // This will always return here is all_costs.size() == 1, or if == 2 and both costs are the same.
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
    total_cost += (cost.first * cost.second);
    count += cost.second;
  }

  return total_cost / count;
}

void MaidAccountMergePolicy::MergePut(const DataNameVariant& data_name,
                                      UnresolvedEntry::Value cost,
                                      const NonEmptyString& serialised_db_value) {
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    uint64_t current_total_size(current_values.first.data * current_values.second.data);
    ++current_values.second.data;
    current_values.first.data = static_cast<int32_t>(
                                    (current_total_size + cost) / current_values.second.data);
    account_db_->Put(std::make_pair(data_name, SerialiseDbValue(current_values)));
  } else {
      // TODO(david/fraser) this will require we send the same message *count* times
      // this should be optimised to handle xmitting the *count*
    DbValue db_value(std::make_pair(AverageCost(cost), Count(1)));
    account_db_->Put(std::make_pair(data_name, SerialiseDbValue(db_value)));
  }
}

void MaidAccountMergePolicy::MergeDelete(const DataNameVariant& data_name,
                                         const NonEmptyString& serialised_db_value) {
  if (!serialised_db_value.IsInitialised()) {
    // No need to check in unresolved_data_, since the corresponding "Put" will already have been
    // marked as "dont_add_to_db".
    return;
  }

  auto current_values(ParseDbValue(serialised_db_value));
  assert(current_values.second.data > 0);
  if (current_values.second.data == 1) {
    account_db_->Delete(data_name);
  } else {
    --current_values.second.data;
    account_db_->Put(std::make_pair(data_name, SerialiseDbValue(current_values)));
  }
}

NonEmptyString MaidAccountMergePolicy::SerialiseDbValue(DbValue db_value) const {
  protobuf::MaidAccountDbValue proto_db_value;
  proto_db_value.set_average_cost(db_value.first.data);
  proto_db_value.set_count(db_value.second.data);
  return NonEmptyString(proto_db_value.SerializeAsString());
}

MaidAccountMergePolicy::DbValue MaidAccountMergePolicy::ParseDbValue(
    NonEmptyString serialised_db_value) const {
  protobuf::MaidAccountDbValue proto_db_value;
  if (!proto_db_value.ParseFromString(serialised_db_value.string()))
    ThrowError(CommonErrors::parsing_error);
  return std::make_pair(AverageCost(proto_db_value.average_cost()), Count(proto_db_value.count()));
}

NonEmptyString MaidAccountMergePolicy::GetFromDb(const DataNameVariant& data_name) {
  NonEmptyString serialised_db_value;
  try {
    serialised_db_value = account_db_->Get(data_name);
  }
  catch(const vault_error&) {}
  return serialised_db_value;
}


}  // namespace vault

}  // namespace maidsafe
