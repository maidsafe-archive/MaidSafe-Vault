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

#ifndef MAIDSAFE_VAULT_DATABASE_MERGE_H_
#define MAIDSAFE_VAULT_DATABASE_MERGE_H_

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
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {
// Account transfer messages passed here
// this class will merge the records to the database supplied
// using the template key / value types to parse
template <typename Key, typename Value, typename DatabasePolicy>
class DataBaseMerge : public Key, public Value, public DatabasePolicy {
 public:
  DataBaseMerge() :
    active_(),
    kv_pair_(),
    unmerged_entry_(),
    unmerged_entries_() {}
  void insert(const nfs::Message& message);
 private:
  DataBaseMerge(const DataBaseMerge&);
  DataBaseMerge& operator=(const DataBaseMerge&);
  Active active_;
  std::tuple<Key, Value> kv_pair_;
  std::tuple<std::tuple<Key, Value>, size_t> unmerged_entry_;
  std::vector<std::tuple<std::tuple<Key, Value>, size_t>> unmerged_entries_;
};

//#include "maidsafe/vault/database_merge.inl"



template <typename Key, typename Value, typename DatabasePolicy>
void DataBaseMerge<Key, Value, DatabasePolicy>::insert(const nfs::Message& /* message */) {
 // here we 
 // 1: get Key and value from message (construct from repeated message field)
 // 2: if Key in Database (drop) 
 // 3: Create kv_pair (unmerged_entry_) and check unmerged_entries_ for this
 // if exists -> increment count unmerged_entries_ for this unmerged_entry_
 //     if count >= group /2 then write to database and delete entry
 // if !exists -> add to container
 // This method should run in an active object I think.
}


}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_DATABASE_MERGE_H_
