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

#ifndef MAIDSAFE_VAULT_KEY_VALUE_MAP_H_
#define MAIDSAFE_VAULT_KEY_VALUE_MAP_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include "maidsafe/common/types.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/routing/close_nodes_change.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

template <typename Key, typename Value>
class KeyValueMap {
 public:
  typedef std::pair<Key, Value> KvPair;
  typedef std::map<NodeId, std::vector<KvPair>> TransferInfo;

  explicit KeyValueMap();
  ~KeyValueMap();

  Value Get(const Key& key);
  // if functor returns DbAction::kDelete, the value is deleted from key_value_map
  std::unique_ptr<Value> Commit(
      const Key& key, std::function<detail::DbAction(std::unique_ptr<Value>& value)> functor);
  TransferInfo GetTransferInfo(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);
  void HandleTransfer(const std::vector<KvPair>& contents);

 private:
  KeyValueMap(const KeyValueMap&);
  KeyValueMap& operator=(const KeyValueMap&);
  KeyValueMap(KeyValueMap&&);
  KeyValueMap& operator=(KeyValueMap&&);
  void Delete(const Key& key);
  void Put(const KvPair& key_value_pair);

  mutable std::mutex mutex_;
  std::map<Key, Value> key_value_map_;
};

template <typename Key, typename Value>
KeyValueMap<Key, Value>::KeyValueMap() : mutex_(), key_value_map_() {
#if defined(__GNUC__) && (!defined(MAIDSAFE_APPLE) && !(defined(_MSC_VER) && _MSC_VER == 1700))
  // Remove this assert if value needs to be copy constructible.
  // this is just a check to avoid copy constructor unless we require it
//  static_assert(!std::is_copy_constructible<Value>::value,
//                "value should not be copy constructible !"); MAID-357
  static_assert(std::is_move_constructible<Value>::value, "value should be move constructible !");
#endif
}

template <typename Key, typename Value>
KeyValueMap<Key, Value>::~KeyValueMap() {
  key_value_map_.clear();
}

template <typename Key, typename Value>
std::unique_ptr<Value> KeyValueMap<Key, Value>::Commit(
    const Key& key, std::function<detail::DbAction(std::unique_ptr<Value>& value)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<Value> value;
  try {
    value.reset(new Value(Get(key)));
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "KeyValueMap<Key, Value>::Commit unknown error "
                  << boost::diagnostic_information(error);
      throw error;
    }
  }
  if (detail::DbAction::kPut == functor(value)) {
    assert(value);
    if (!value)
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::null_pointer));

    LOG(kInfo) << "KeyValueMap<Key, Value>::Commit putting entry";
    Put(KvPair(key, Value(std::move(*value))));
  } else {
    LOG(kInfo) << "KeyValueMap<Key, Value>::Commit deleting entry";
    assert(value);
    Delete(key);
    return value;
  }
  return nullptr;
}

// option 1 : Fire functor here with check_holder_result.new_holder & the corresponding value
// option 2 : create a map<NodeId, std::vector<std::pair<Key, value>>> and return after pruning
template <typename Key, typename Value>
typename KeyValueMap<Key, Value>::TransferInfo KeyValueMap<Key, Value>::GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Key> prune_vector;
  TransferInfo transfer_info;
  LOG(kVerbose) << "KeyValueMap::GetTransferInfo";
  for (auto& key_value_pair : key_value_map_) {
    auto check_holder_result = close_nodes_change->CheckHolders(
          NodeId(key_value_pair.first.name.string()));
    if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
      LOG(kVerbose) << "KeyValueMap::GetTransferInfo in range";
      if (check_holder_result.new_holder == NodeId())
        continue;
      LOG(kVerbose) << "KeyValueMap::GetTransferInfo having new holder "
                    << check_holder_result.new_holder;
      auto found_itr = transfer_info.find(check_holder_result.new_holder);
      if (found_itr != transfer_info.end()) {
        LOG(kInfo) << "KeyValueMap::GetTransferInfo add into transfering account "
                   << HexSubstr(key_value_pair.first.name.string())
                   << " to " << check_holder_result.new_holder;
        found_itr->second.push_back(key_value_pair);
      } else {  // create
        LOG(kInfo) << "KeyValueMap::GetTransferInfo create transfering account "
                   << HexSubstr(key_value_pair.first.name.string())
                   << " to " << check_holder_result.new_holder;
        std::vector<KvPair> kv_pair;
        kv_pair.push_back(key_value_pair);
        transfer_info.insert(std::make_pair(check_holder_result.new_holder, std::move(kv_pair)));
      }
    } else {
      VLOG(VisualiserAction::kRemoveAccount, key_value_pair.first.name);
      prune_vector.push_back(key_value_pair.first);
    }
  }

  for (const auto& key : prune_vector)
    key_value_map_.erase(key);
  return transfer_info;
}

// Ignores values which are already in db
template <typename Key, typename Value>
void KeyValueMap<Key, Value>::HandleTransfer(const std::vector<std::pair<Key, Value>>& contents) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& kv_pair : contents) {
    try {
      Get(kv_pair.first);
    }
    catch (const maidsafe_error& error) {
      LOG(kInfo) << error.what();
      if ((error.code() != make_error_code(CommonErrors::no_such_element)) &&
          (error.code() != make_error_code(VaultErrors::no_such_account)))
        throw;  // For db errors
      else
        Put(kv_pair);
    }
  }
}

template <typename Key, typename Value>
Value KeyValueMap<Key, Value>::Get(const Key& key) {
  auto itr(key_value_map_.find(key));
  if (itr == key_value_map_.end())
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  return itr->second;
}

template <typename Key, typename Value>
void KeyValueMap<Key, Value>::Put(const KvPair& key_value_pair) {
  key_value_map_[key_value_pair.first] = key_value_pair.second;
}

template <typename Key, typename Value>
void KeyValueMap<Key, Value>::Delete(const Key& key) {
  key_value_map_.erase(key);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_VALUE_MAP_H_
