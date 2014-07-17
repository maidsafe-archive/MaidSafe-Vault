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

#include "leveldb/db.h"

#include "maidsafe/common/types.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/routing/close_nodes_change.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"


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
  std::unique_ptr<leveldb::DB> leveldb_;
};

template <typename Key, typename Value>
Db<Key, Value>::Db(const boost::filesystem::path& db_path)
    : kDbPath_(db_path), mutex_(), leveldb_() {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, kDbPath_.string(), &db));
  if (!status.ok())
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
  assert(leveldb_);
#if defined(__GNUC__) && (!defined(MAIDSAFE_APPLE) && !(defined(_MSC_VER) && _MSC_VER == 1700))
  // Remove this assert if value needs to be copy constructible.
  // this is just a check to avoid copy constructor unless we require it
  static_assert(!std::is_copy_constructible<Value>::value,
                "value should not be copy constructible !");
  static_assert(std::is_move_constructible<Value>::value, "value should be move constructible !");
#endif
}

template <typename Key, typename Value>
Db<Key, Value>::~Db() {
  try {
    leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
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
      throw error;  // For db errors
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
  {
    LOG(kVerbose) << "Db::GetTransferInfo";
    std::unique_ptr<leveldb::Iterator> db_iter(leveldb_->NewIterator(leveldb::ReadOptions()));
    for (db_iter->SeekToFirst(); db_iter->Valid(); db_iter->Next()) {
      Key key(typename Key::FixedWidthString(db_iter->key().ToString()));
      auto check_holder_result = close_nodes_change->CheckHolders(NodeId(key.name.string()));
      if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
        LOG(kVerbose) << "Db::GetTransferInfo in range";
        if (check_holder_result.new_holders.size() != 0) {
          for (size_t index(0); index < check_holder_result.new_holders.size(); ++index)
            if (index == 0)
              LOG(kVerbose) << "Db::GetTransferInfo having new node "
                            << DebugId(check_holder_result.new_holders.at(index));
            else
              LOG(kError) << "Db::GetTransferInfo unprocessed new node "
                          << DebugId(check_holder_result.new_holders.at(index));
          // assert(check_holder_result.new_holders.size() == 1);
          auto found_itr = transfer_info.find(check_holder_result.new_holders.at(0));
          if (found_itr != transfer_info.end()) {
            found_itr->second.push_back(std::make_pair(key, Value(db_iter->value().ToString())));
          } else {  // create
            LOG(kInfo) << "Db::GetTransferInfo transfering account " << HexSubstr(key.name.string())
                       << " to " << DebugId(check_holder_result.new_holders.at(0));
            std::vector<KvPair> kv_pair;
            kv_pair.push_back(std::make_pair(key, Value(db_iter->value().ToString())));
            transfer_info.insert(
                std::make_pair(check_holder_result.new_holders.at(0), std::move(kv_pair)));
          }
        }
      } else {
        VLOG(VisualiserAction::kRemoveAccount, key.name);
        prune_vector.push_back(db_iter->key().data());
      }
    }
  }

  for (const auto& key_string : prune_vector)
    leveldb_->Delete(leveldb::WriteOptions(), key_string);  // Ignore Delete failure here ?
  return transfer_info;
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
        throw error;  // For db errors
      else
        Put(kv_pair);
    }
  }
}

// throws on level-db errors other than key not found
template <typename Key, typename Value>
Value Db<Key, Value>::Get(const Key& key) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value_string;
  leveldb::Status status(
      leveldb_->Get(read_options, key.ToFixedWidthString().string(), &value_string));
  if (status.ok()) {
    assert(!value_string.empty());
    return Value(value_string);
  } else if (status.IsNotFound()) {
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  }
  BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
}

template <typename Key, typename Value>
void Db<Key, Value>::Put(const KvPair& key_value_pair) {
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       key_value_pair.first.ToFixedWidthString().string(),
                                       key_value_pair.second.Serialise()));
  if (!status.ok()) {
    LOG(kError) << "Db<Key, Value>::Put incorrect leveldb::Status";
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
  }
}

template <typename Key, typename Value>
void Db<Key, Value>::Delete(const Key& key) {
  leveldb::Status status(
      leveldb_->Delete(leveldb::WriteOptions(), key.ToFixedWidthString().string()));
  if (!status.ok()) {
    LOG(kError) << "Db<Key, Value>::Delete incorrect leveldb::Status";
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
