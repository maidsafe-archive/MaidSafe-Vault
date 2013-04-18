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

#include "maidsafe/common/error.h"

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
                                                                     //todo - check if throws if not exists
  auto serialised_db_value(db_->Get(data_name_and_action.first));
  if (data_name_and_action.second == nfs::MessageAction::kPut)
    MergePut(data_name_and_action.first, cost, serialised_db_value);
  else if (data_name_and_action.second == nfs::MessageAction::kDelete)
    MergeDelete(data_name_and_action.first, serialised_db_value);
  else
    ThrowError(CommonErrors::invalid_parameter);
}

void MaidAccountMergePolicy::MergePut(const DataNameVariant& data_name,
                                      UnresolvedAction::Value cost,
                                      const NonEmptyString& serialised_db_value) {
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    uint64_t current_total_size(current_values.first.data * current_values.second.data);
    ++current_values.second.data;
    current_values.first.data = static_cast<int32_t>(
                                    (current_total_size + cost) / current_values.second.data);
    db_->Put(data_name, SerialiseDbValue(current_values));
  } else {
    DbValue db_value(std::make_pair(AverageCost(cost), Count(1)));
    db_->Put(data_name, SerialiseDbValue(db_value));
  }
}

void MaidAccountMergePolicy::MergeDelete(const DataNameVariant& data_name,
                                         const NonEmptyString& serialised_db_value) {
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    assert(current_values.second.data > 0);
    if (current_values.second.data == 1) {
      db_->Delete(data_name);
    } else {
      --current_values.second.data;
      db_->Put(data_name, SerialiseDbValue(current_values));
    }
  } else {
                //todo - check if we should look in unresolved_data_ here.  If so, modify comment in header
    ThrowError(CommonErrors::no_such_element);
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

}  // namespace vault

}  // namespace maidsafe
