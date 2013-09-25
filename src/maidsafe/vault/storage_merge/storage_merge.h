/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_STORAGE_MERGE_H_
#define MAIDSAFE_VAULT_STORAGE_MERGE_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <tuple>
#include <set>

#include "maidsafe/common/error.h"
#include "maidsafe/common/active.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/storage_merge/storage_merge.pb.h"

namespace maidsafe {

namespace vault {

template <typename Key, typename Value, typename StoragePolicy>
class StorageMerge : public Key, public Value, public StoragePolicy {
 public:
  StorageMerge() : active_(), unmerged_entries_() {}
  //  void insert(const nfs::Message& message);
  typedef std::tuple<std::tuple<Key, Value>, NodeId> UnmergedEntry;

 private:
  StorageMerge(const StorageMerge&);
  StorageMerge& operator=(const StorageMerge&);
  Active active_;
  //                                Key  dbvalue             sender
  std::vector<std::tuple<std::tuple<Key, Value>, std::set<NodeId>>> unmerged_entries_;
  bool KeyExist(const Key& key);
  typename std::vector<std::tuple<std::tuple<Key, Value>, std::set<NodeId>>>::iterator
      FindUnmergedEntry(const UnmergedEntry& unmerged_entry);
};

template <typename Key, typename Value, typename StoragePolicy>
bool StorageMerge<Key, Value, StoragePolicy>::KeyExist(const Key& key) {
  try {
    StoragePolicy::Get(key);
  }
  catch (std::exception&) {
    return false;
  }
  return true;
}

template <typename Key, typename Value, typename StoragePolicy>
typename std::vector<std::tuple<std::tuple<Key, Value>, std::set<NodeId>>>::iterator
StorageMerge<Key, Value, StoragePolicy>::FindUnmergedEntry(const UnmergedEntry& unmerged_entry) {
  return std::find(std::begin(unmerged_entries_), std::end(unmerged_entries_),
                   [unmerged_entry](const std::tuple<std::tuple<Key, Value>, std::set<NodeId>> &
                                    entry) { return (std::get<0>(entry) == unmerged_entry); });
}

// template <typename Key, typename Value, typename StoragePolicy>
// void StorageMerge<Key, Value, StoragePolicy>::insert(const nfs::Message& message) {
//  if (static_cast<nfs::MessageAction>(message.data().action) !=
//      nfs::MessageAction::kAccountTransfer)
//    ThrowError(CommonErrors::invalid_parameter);
//  protobuf::StorageMerge storage_proto;
//  storage_proto.ParseFromString(message.data().content.string());
//  for (const auto& record: storage_proto.records()) {
//    auto key(record.key());
//    if (KeyExist(key))
//      continue;
//    auto value(record.value());
//    auto unmerged_entry(std::make_tuple(key, value));
//    auto found = FindUnmergedEntry(std::make_tuple(key, value));
//    if (found == std::end(unmerged_entries_)) {
//      unmerged_entries_.emplace_back(std::make_tuple(std::make_tuple(key, value),
//                                                   std::set<NodeId> { message.source().node_id
// }));
//    } else {
//      std::get<1>(found).insert(message.source());
//      if (std::get<1>(found).size() >=
//          static_cast<size_t>((routing::Parameters::node_group_size / 2))) {
//        StoragePolicy::Put(key, value);
//        std::get<1>(found).erase();
//      }
//    }
//  }
//}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STORAGE_MERGE_H_
