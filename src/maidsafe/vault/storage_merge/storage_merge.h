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

#ifndef MAIDSAFE_VAULT_STORAGE_MERGE_H_
#define MAIDSAFE_VAULT_STORAGE_MERGE_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <tuple>

#include "leveldb/db.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/active.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {
// Account transfer messages passed here
// this class will merge the records to the storageatabase supplied
// using the template key / value types to parse
template <typename Key, typename Value, typename StoragePolicy>
class StorageMerge : public Key, public Value, public StoragePolicy {
 public:
  StorageMerge() :
    active_(),
    kv_pair_(),
    unmerged_entry_(),
    unmerged_entries_() {}
  void insert(const nfs::Message& message);
 private:
  StorageMerge(const StorageMerge&);
  StorageMerge& operator=(const StorageMerge&);
  bool KeyExist(const Key& key);
  Active active_;
  std::tuple<Key, Value> kv_pair_;
  std::tuple<std::tuple<Key, Value>, NodeId> unmerged_entry_;
  //                                Key  dbvalue                        sender
  std::vector<std::tuple<std::tuple<Key, Value>, std::vector<NodeId>>> unmerged_entries_;
};

//#include "maidsafe/vault/Storage_merge.inl"

template <typename Key, typename Value, typename StoragePolicy>
void StorageMerge<Key, Value, StoragePolicy>::insert(const nfs::Message& message) {
  if (static_cast<nfs::MessageAction>(message.data().action) !=
      nfs::MessageAction::kAccountTransfer)
    ThrowError(CommonErrors::invalid_parameter);
  for (const auto& record: message.data().content) {
    auto key(record.key());
    if (KeyExist(key))
      continue;
    auto value(record.value());
    auto record(std::make_pair(key, value));

  }

 // here we
 // 1: get Key and value from message (construct from repeated message field)
 // 2: if Key in Storage (drop)
 // 3: Create kv_pair (unmerged_entry_) and check unmerged_entries_ for this
 // if exists -> increment count unmerged_entries_ for this unmerged_entry_
 //     if count >= group /2 then write to Storage and delete entry
 // if !exists -> add to container
 // This method should run in an active object I think.
}


}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_STORAGE_MERGE_H_
