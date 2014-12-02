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

#ifndef MAIDSAFE_VAULT_DB_H_
#define MAIDSAFE_VAULT_DB_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <map>

#include "boost/filesystem.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/routing/close_nodes_change.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/database_operations.h"


namespace maidsafe {

namespace vault {

template <typename Key, typename Value>
class Db {
 public:
  typedef std::pair<Key, Value> KvPair;
  typedef std::map<NodeId, std::vector<KvPair>> TransferInfo;

  explicit Db(const boost::filesystem::path& db_path);
  ~Db();

  Value Get(const Key& key);
  // if functor returns DbAction::kDelete, the value is deleted from db
  std::unique_ptr<Value> Commit(
      const Key& key, std::function<detail::DbAction(std::unique_ptr<Value>& value)> functor);
  TransferInfo GetTransferInfo(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);
  std::vector<Key> GetTargets(const PmidName& pmid_node);
  void HandleTransfer(const std::vector<KvPair>& contents);

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);
  void Delete(const Key& key);
  void Put(const KvPair& key_value_pair);

  const boost::filesystem::path kDbPath_;
  mutable std::mutex mutex_;
  std::unique_ptr<VaultDataBase> sqlitedb_;
};

template <typename Key, typename Value>
Db<Key, Value>::Db(const boost::filesystem::path& db_path)
    : kDbPath_(db_path), mutex_(), sqlitedb_() {
  sqlitedb_.reset(new VaultDataBase(kDbPath_));
#if defined(__GNUC__) && (!defined(MAIDSAFE_APPLE) && !(defined(_MSC_VER) && _MSC_VER == 1700))
  // Remove this assert if value needs to be copy constructible.
  // this is just a check to avoid copy constructor unless we require it
//  static_assert(!std::is_copy_constructible<Value>::value,
//                "value should not be copy constructible !"); MAID-357
  static_assert(std::is_move_constructible<Value>::value, "value should be move constructible !");
#endif
}

template <typename Key, typename Value>
Db<Key, Value>::~Db() {
  try {
    sqlitedb_.reset();
    boost::filesystem::remove_all(kDbPath_);
  }
  catch (const std::exception& e) {
    LOG(kError) << "Failed to remove db : " << boost::diagnostic_information(e);
  }
}

template <typename Key, typename Value>
std::unique_ptr<Value> Db<Key, Value>::Commit(
    const Key& key, std::function<detail::DbAction(std::unique_ptr<Value>& value)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<Value> value;
  try {
    value.reset(new Value(Get(key)));
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "Db<Key, Value>::Commit unknown db error "
                  << boost::diagnostic_information(error);
      throw;  // For db errors
    }
  }
  if (detail::DbAction::kPut == functor(value)) {
    assert(value);
    if (!value)
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::null_pointer));

    LOG(kInfo) << "Db<Key, Value>::Commit putting entry";
    Put(KvPair(key, Value(std::move(*value))));
  } else {
    LOG(kInfo) << "Db<Key, Value>::Commit deleting entry";
    assert(value);
    Delete(key);
    return value;
  }
  return nullptr;
}

// option 1 : Fire functor here with check_holder_result.new_holder & the corresponding value
// option 2 : create a map<NodeId, std::vector<std::pair<Key, value>>> and return after pruning
template <typename Key, typename Value>
typename Db<Key, Value>::TransferInfo Db<Key, Value>::GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> prune_vector;
  TransferInfo transfer_info;
  LOG(kVerbose) << "Db::GetTransferInfo";
  std::pair<std::string, std::string> db_iter;
  while (sqlitedb_->SeekNext(db_iter)) {
    Key key(typename Key::FixedWidthString(db_iter.first));
    auto check_holder_result = close_nodes_change->CheckHolders(NodeId(key.name.string()));
    if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
      LOG(kVerbose) << "Db::GetTransferInfo in range";
      if (check_holder_result.new_holder == NodeId())
        continue;
      LOG(kVerbose) << "Db::GetTransferInfo having new holder " << check_holder_result.new_holder;
      auto found_itr = transfer_info.find(check_holder_result.new_holder);
      if (found_itr != transfer_info.end()) {
        LOG(kInfo) << "Db::GetTransferInfo add into transfering account "
                   << HexSubstr(key.name.string()) << " to " << check_holder_result.new_holder;
        found_itr->second.push_back(std::make_pair(key, Value(db_iter.second)));
      } else {  // create
        LOG(kInfo) << "Db::GetTransferInfo create transfering account "
                   << HexSubstr(key.name.string()) << " to " << check_holder_result.new_holder;
        std::vector<KvPair> kv_pair;
        kv_pair.push_back(std::make_pair(key, Value(db_iter.second)));
        transfer_info.insert(std::make_pair(check_holder_result.new_holder, std::move(kv_pair)));
      }
    } else {
      VLOG(VisualiserAction::kRemoveAccount, key.name);
      prune_vector.push_back(db_iter.first);
    }
  }

  for (const auto& key_string : prune_vector)
    sqlitedb_->Delete(key_string);  // Ignore Delete failure here ?
  return transfer_info;
}

template <typename Key, typename Value>
std::vector<Key> Db<Key, Value>::GetTargets(const PmidName& pmid_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<Key> result;
  std::pair<std::string, std::string> db_iter;
  while (sqlitedb_->SeekNext(db_iter)) {
    Value value(db_iter.second);
    if (value.HasTarget(pmid_name)) {
      Key key(typename Key::FixedWidthString(db_iter.first));
      result.push_back(std::move(key));
    }
  }
  return result;
}

// Ignores values which are already in db
template <typename Key, typename Value>
void Db<Key, Value>::HandleTransfer(const std::vector<std::pair<Key, Value>>& contents) {
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
Value Db<Key, Value>::Get(const Key& key) {
  std::string value_string;
  sqlitedb_->Get(key.ToFixedWidthString().string(), value_string);
  if (value_string.empty())
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  return Value(value_string);
}

template <typename Key, typename Value>
void Db<Key, Value>::Put(const KvPair& key_value_pair) {
  sqlitedb_->Put(key_value_pair.first.ToFixedWidthString().string(),
                 key_value_pair.second.Serialise());
}

template <typename Key, typename Value>
void Db<Key, Value>::Delete(const Key& key) {
  sqlitedb_->Delete(key.ToFixedWidthString().string());
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
