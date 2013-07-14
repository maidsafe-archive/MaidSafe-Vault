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

#ifndef MAIDSAFE_VAULT_MANAGER_DB_INL_H_
#define MAIDSAFE_VAULT_MANAGER_DB_INL_H_

#include "maidsafe/vault/manager_db.h"

#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"


namespace maidsafe {

namespace vault {

template<typename Key, typename Value>
ManagerDb<Key, Value>::ManagerDb()
    : kDbPath_(boost::filesystem::unique_path()),
      mutex_(),
      leveldb_() {
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

template<typename Key, typename Value>
ManagerDb<Key, Value>::~ManagerDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

template<typename Key, typename Value>
void ManagerDb<Key, Value>::Put(const KvPair& key_value_pair) {
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       key_value_pair.first.Serialise(),
                                       key_value_pair.second.Serialise()->string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template<typename Key, typename Value>
void ManagerDb<Key, Value>::Delete(const Key& key) {
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), key.Serialise()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template<typename Key, typename Value>
Value ManagerDb<Key, Value>::Get(const Key& key) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, key.Serialise(), &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return Value(typename Value::serialised_type(NonEmptyString(value)));
}

// TODO(Team) This can be optimise by returning const_iterators.
// getkeys close to XXX would be better
template<typename Key, typename Value>
auto ManagerDb<Key, Value>::GetKeys() ->decltype(std::vector<Key>()) {
  std::vector<Key> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  for (iter->SeekToFirst(); iter->Valid(); iter->Next())
    return_vector.emplace_back(Key(iter->key()));
  return return_vector;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MANAGER_DB_INL_H_
