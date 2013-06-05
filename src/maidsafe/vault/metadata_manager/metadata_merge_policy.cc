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
#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/metadata_manager/metadata_value.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

MetadataMergePolicy::MetadataMergePolicy(ManagerDb<MetadataManager> *metadata_db)
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

void MetadataMergePolicy::Merge(const UnresolvedEntry& unresolved_entry) {
  if (unresolved_entry.key.second == nfs::MessageAction::kPut &&
      !unresolved_entry.dont_add_to_db) {
    auto data_size(GetDataSize(unresolved_entry));
    if (!data_size)
      MergePut(unresolved_entry.key.first, data_size);
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
//    MergeDelete(unresolved_entry.key.first, serialised_db_value);
  } else {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

int MetadataMergePolicy::GetDataSize(
    const UnresolvedEntry& unresolved_entry) const {
  assert(unresolved_entry.key.second == nfs::MessageAction::kPut &&
         !unresolved_entry.dont_add_to_db);
  std::map<int, size_t> all_data_size;
  auto most_frequent_itr(std::end(unresolved_entry.messages_contents));
  size_t most_frequent(0);
  for (auto itr(std::begin(unresolved_entry.messages_contents));
       itr != std::end(unresolved_entry.messages_contents); ++itr) {
    if ((*itr).value.get().data_size) {
      size_t this_value_count(++all_data_size[itr->value.get().data_size]);
      if (this_value_count > most_frequent) {
        most_frequent = this_value_count;
        most_frequent_itr = itr;
      }
    }
  }

  if (all_data_size.empty())
    ThrowError(CommonErrors::unknown);
  if (most_frequent > static_cast<size_t>(routing::Parameters::node_group_size / 2))
    return most_frequent_itr->value.get().data_size;

  if (unresolved_entry.messages_contents.size() == routing::Parameters::node_group_size) {
    assert(false && "Invalid datasize from peers");
    ThrowError(CommonErrors::unknown);
  }

  return 0;
}

void MetadataMergePolicy::MergePut(const DataNameVariant& data_name,
                                   int data_size) {
  Metadata metadata(data_name, metadata_db_, data_size);
  ++(*metadata.value_.subscribers);
  metadata.SaveChanges(metadata_db_);
}

void MetadataMergePolicy::MergeDelete(const DataNameVariant& /*data_name*/,
                                      const NonEmptyString& /*serialised_db_value*/) {
}

}  // namespace vault

}  // namespace maidsafe
