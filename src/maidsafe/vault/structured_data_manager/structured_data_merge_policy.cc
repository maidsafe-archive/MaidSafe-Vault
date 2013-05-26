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

#include <set>

#include "maidsafe/common/error.h"

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/vault/structured_data_manager/structured_data_db.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace maidsafe {

namespace vault {

StructuredDataMergePolicy::StructuredDataMergePolicy(StructuredDataDb* db)
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

//void StructuredDataMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
//  auto serialised_db_value(GetFromDb(unresolved_entry.key.first));
//  switch (nfs::MessageAction) {
//    case(kGetBranch) :

//  }
//  if (unresolved_entry.key.second == nfs::MessageAction::kGetBranch) {
//    MergePut(unresolved_entry.key.first, MergedCost(unresolved_entry), serialised_db_value);
//  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
//    MergeDelete(unresolved_entry.key.first, serialised_db_value);
//  } else {
//    ThrowError(CommonErrors::invalid_parameter);
//  }
//}


//void StructuredDataMergePolicy::MergePut(const DataNameVariant& data_name,
//                                      UnresolvedEntry::Value cost,
//                                      const NonEmptyString& serialised_db_value) {
//  if (serialised_db_value.IsInitialised()) {
//    auto current_values(ParseDbValue(serialised_db_value));
//    uint64_t current_total_size(current_values.first.data * current_values.second.data);
//    ++current_values.second.data;
//    current_values.first.data =
//        static_cast<int32_t>((current_total_size + cost) / current_values.second.data);
//    db_->Put(std::make_pair(data_name, SerialiseDbValue(current_values)));
//  } else {
//      // TODO(david/fraser) this will require we send the same message *count* times
//      // this should be optimised to handle xmitting the *count*
//    DbValue db_value(std::make_pair(AverageCost(cost), Count(1)));
//    db_->Put(std::make_pair(data_name, SerialiseDbValue(db_value)));
//  }
//}

//void StructuredDataMergePolicy::MergeDelete(const DataNameVariant& data_name,
//                                         const NonEmptyString& serialised_db_value) {
//  if (!serialised_db_value.IsInitialised()) {
//    // No need to check in unresolved_data_, since the corresponding "Put" will already have been
//    // marked as "dont_add_to_db".
//    return;
//  }

//  auto current_values(ParseDbValue(serialised_db_value));
//  assert(current_values.second.data > 0);
//  if (current_values.second.data == 1) {
//    db_->Delete(data_name);
//  } else {
//    --current_values.second.data;
//    db_->Put(std::make_pair(data_name, SerialiseDbValue(current_values)));
//  }
//}

//NonEmptyString StructuredDataMergePolicy::SerialiseDbValue(DbValue db_value) const {
//  protobuf::MaidAccountDbValue proto_db_value;
//  proto_db_value.set_average_cost(db_value.first.data);
//  proto_db_value.set_count(db_value.second.data);
//  return NonEmptyString(proto_db_value.SerializeAsString());
//}

//StructuredDataMergePolicy::DbValue StructuredDataMergePolicy::ParseDbValue(
//    NonEmptyString serialised_db_value) const {
//  return StructuredDataVersions(serialised_db_value);
//}

//NonEmptyString StructuredDataMergePolicy::GetFromDb(const DbKey &db_key) {
//  NonEmptyString serialised_db_value;
//  try {
//    serialised_db_value = db_->Get(db_key);
//  }
//  catch(const vault_error&) {}
//  return serialised_db_value;
//}

}  // namespace vault

}  // namespace maidsafe
