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

#include "maidsafe/vault/metadata_manager/metadata_merge_policy.h"

#include <set>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

MetadataMergePolicy::MetadataMergePolicy(MetadataDb * metadata_db)
    : unresolved_data_(),
      metadata_db_(metadata_db) {}

MetadataMergePolicy::MetadataMergePolicy(MetadataMergePolicy&& other)
    : unresolved_data_(std::move(other.unresolved_data_)),
      metadata_db_(std::move(other.metadata_db_)) {}

MetadataMergePolicy& MetadataMergePolicy::operator=(MetadataMergePolicy&& other) {
  unresolved_data_ = std::move(other.unresolved_data_);
  metadata_db_ = std::move(other.metadata_db_);
  return *this;
}

void MetadataMergePolicy::Merge(const UnresolvedEntry& /*unresolved_entry*/) {
//  auto serialised_db_value(GetFromDb(unresolved_entry.key.first));
//  if (unresolved_entry.key.second == nfs::MessageAction::kPut &&
//      !unresolved_entry.dont_add_to_db) {
//    MergePut(unresolved_entry.key.first, MergedCost(unresolved_entry), serialised_db_value);
//  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
//    MergeDelete(unresolved_entry.key.first, serialised_db_value);
//  } else {
//    ThrowError(CommonErrors::invalid_parameter);
//  }
}

void MetadataMergePolicy::MergePut(const DataNameVariant& /*data_name*/,
                                      UnresolvedEntry::Value /*cost*/,
                                      const NonEmptyString& /*serialised_db_value*/) {
}

void MetadataMergePolicy::MergeDelete(const DataNameVariant& /*data_name*/,
                                         const NonEmptyString& /*serialised_db_value*/) {
}

}  // namespace vault

}  // namespace maidsafe
