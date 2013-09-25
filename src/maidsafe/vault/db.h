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

#include "boost/filesystem.hpp"
#include "boost/optional/optional.hpp"

#include "leveldb/db.h"

#include "maidsafe/routing/matrix_change.h"

namespace maidsafe {

namespace vault {

template <typename Key, typename Value>
class Db {
 public:
  typedef std::pair<Key, Value> KvPair;
  typedef std::map<NodeId, std::vector<KvPair>> TransferInfo;

  Db();
  ~Db();

  boost::optional<Value> Get(const Key& key);
  void Commit(const Key& key, std::function<void(boost::optional<Value>& value)> functor);
  TransferInfo GetTransferInfo(std::shared_ptr<routing::MatrixChange> matrix_change);
  void HandleTransfer(const std::vector<KvPair>& contents);

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);
  void Delete(const Key& key);
  void Put(const KvPair& key_value_pair);
  boost::optional<Value> GetValue(const Key& key);

  const boost::filesystem::path kDbPath_;
  mutable std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

template <typename Key, typename Value>
Db<Key, Value>::Db()
    : kDbPath_(boost::filesystem::unique_path()), mutex_(), leveldb_() {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, kDbPath_.string(), &db));
  if (!status.ok())
    ThrowError(CommonErrors::filesystem_io_error);
  leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
  assert(leveldb_);
}

template <typename Key, typename Value>
Db<Key, Value>::~Db() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

template <typename Key, typename Value>
boost::optional<Value> Db<Key, Value>::Get(const Key& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  return GetValue(key);
}

template <typename Key, typename Value>
void Db<Key, Value>::Commit(const Key& key,
                            std::function<void(boost::optional<Value>& value)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  boost::optional<Value> value(GetValue(key));
  bool value_found_in_db(value);
  functor(value);
  if (value)
    Put(KvPair(key, Value(*value)));
  else if (value_found_in_db)
    Delete(key);
}

// option 1 : Fire functor here with check_holder_result.new_holder & the corresponding value
// option 2 : create a map<NodeId, std::vector<std::pair<Key, value>>> and return after pruning
template <typename Key, typename Value>
typename Db<Key, Value>::TransferInfo Db<Key, Value>::GetTransferInfo(
    std::shared_ptr<routing::MatrixChange> matrix_change) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> prune_vector;
  TransferInfo transfer_info;
  {
    std::unique_ptr<leveldb::Iterator> db_iter(leveldb_->NewIterator(leveldb::ReadOptions()));
    for (db_iter->SeekToFirst(); db_iter->Valid(); db_iter->Next()) {
      Key key(Key::FixedWidthString(db_iter->key()));
      auto check_holder_result = matrix_change->CheckHolders(NodeId(key->string()));
      if (check_holder_result.proximity_status != routing::GroupRangeStatus::kInRange) {
        if (check_holder_result.new_holders.size() != 0) {
          assert(check_holder_result.new_holders.size() == 1);
          auto found_itr = transfer_info.find(check_holder_result.new_holders.at(0));
          if (found_itr != transfer_info.end()) {
            found_itr->second.push_back(std::make_pair(key, Value(db_iter->value)));
          } else {  // create
            std::vector<KvPair> kv_pair;
            kv_pair.push_back(std::make_pair(key, Value(db_iter->value)));
            transfer_info.insert(std::make_pair(check_holder_result.new_holders.at(0), kv_pair));
          }
        }
      } else {
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
    if (!GetValue(kv_pair.first))
      Put(kv_pair);
  }
}

// private members
// throws on level-db errors other than key not found
template <typename Key, typename Value>
boost::optional<Value> Db<Key, Value>::GetValue(const Key& key) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value_string;
  leveldb::Status status(
      leveldb_->Get(read_options, key.ToFixedWidthString().string(), &value_string));
  if (status.ok()) {
    assert(!value_string.empty());
    return boost::optional<Value>(typename Value::serialised_type((NonEmptyString(value_string))));
  } else if (status.IsNotFound()) {
    return boost::optional<Value>();
  }
  ThrowError(VaultErrors::failed_to_handle_request);
  return boost::optional<Value>();
}

template <typename Key, typename Value>
void Db<Key, Value>::Put(const KvPair& key_value_pair) {
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       key_value_pair.first.ToFixedWidthString().string(),
                                       key_value_pair.second.Serialise()->string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template <typename Key, typename Value>
void Db<Key, Value>::Delete(const Key& key) {
  leveldb::Status status(
      leveldb_->Delete(leveldb::WriteOptions(), key.ToFixedWidthString().string()));
  if (status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
