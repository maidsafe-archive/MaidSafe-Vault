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
    MergePut(unresolved_entry.key.first.name(), GetDataSize(unresolved_entry));
  } else if (unresolved_entry.key.second == nfs::MessageAction::kDelete) {
    MergeDelete(unresolved_entry.key.first.name(), GetDataSize(unresolved_entry));
  } else if (unresolved_entry.key.second == nfs::MessageAction::kAccountTransfer) {
    MergeRecordTransfer(unresolved_entry);
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

void MetadataMergePolicy::MergePut(const DataNameVariant& data_name, int data_size) {
  if (data_size != 0) {
    Metadata metadata(data_name, metadata_db_, data_size);
    ++(*metadata.value_.subscribers);
    metadata.SaveChanges(metadata_db_);
  }
}

void MetadataMergePolicy::MergeDelete(const DataNameVariant& data_name, int data_size) {
  if (data_size != 0) {
    Metadata metadata(data_name, metadata_db_, data_size);
    ++(*metadata.value_.subscribers);
    metadata.SaveChanges(metadata_db_);
  }
}

std::vector<MetadataMergePolicy::UnresolvedEntry> MetadataMergePolicy::MergeRecordTransfer(
    const UnresolvedEntry& unresolved_entry) {
  std::vector<UnresolvedEntry> extra_unresolved_data;
  // Merge Size
  int size(GetDataSize(unresolved_entry));
  MetadataValue metadata_value(size);
  // Merge subscribers
  auto min_subscriber(std::min_element(
      unresolved_entry.messages_contents.begin(),
      unresolved_entry.messages_contents.end(),
      [](const UnresolvedEntry::MessageContent& lhs, const UnresolvedEntry::MessageContent& rhs) {
          return (*lhs.value->subscribers < *rhs.value->subscribers);
      }));
  *metadata_value.subscribers = *min_subscriber->value->subscribers;

  // Creating unresolved entry for unresolved subscribers
  for (const auto& message_content : unresolved_entry.messages_contents) {
    if (*message_content.value->subscribers > *metadata_value.subscribers) {
      auto extra_subscribers(*message_content.value->subscribers - *metadata_value.subscribers);
      MetadataValue incremental_value(metadata_value.data_size);
      *incremental_value.subscribers = 1;
      for (auto i(0); i != extra_subscribers; ++i) {  // FIXME need different entry id ?
        UnresolvedEntry entry(std::make_pair(unresolved_entry.key.first, nfs::MessageAction::kPut),
                              incremental_value, message_content.peer_id);
        extra_unresolved_data.push_back(entry);
      }
    }
  }

  // Online pmid_name
  std::map<PmidName, uint16_t> online_pmid_frequencies;
  std::map<PmidName, uint16_t> offline_pmid_frequencies;
  for (const auto& message_content : unresolved_entry.messages_contents) {
    for (const auto& online_pmid_name : message_content.value->online_pmid_name)
      ++online_pmid_frequencies[online_pmid_name];
    for (const auto& offline_pmid_name : message_content.value->offline_pmid_name)
      ++offline_pmid_frequencies[offline_pmid_name];
  }
  for (const auto& i : online_pmid_frequencies)
    if (i.second >= routing::Parameters::node_group_size - 1U)
      metadata_value.online_pmid_name.insert(i.first);

  for (const auto& i : offline_pmid_frequencies)
    if (i.second >= routing::Parameters::node_group_size - 1U)
      metadata_value.offline_pmid_name.insert(i.first);
  // FIXME need to return unresolved pmid_names to Ping/Get data
  Metadata metadata(unresolved_entry.key.first.name(), metadata_db_, metadata_value.data_size);
  metadata.value_ = metadata_value;   // Overwriting DB here
  metadata.SaveChanges(metadata_db_);
  return extra_unresolved_data;
}

}  // namespace vault

}  // namespace maidsafe
