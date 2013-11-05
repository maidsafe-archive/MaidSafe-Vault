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

#ifndef MAIDSAFE_VAULT_GROUP_DB_H_
#define MAIDSAFE_VAULT_GROUP_DB_H_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/optional/optional.hpp"
#include "leveldb/db.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {

// All public methods provide strong exception guarantee
template <typename Persona>
class GroupDb {
 public:
  typedef typename Persona::GroupName GroupName;
  typedef typename Persona::Key Key;
  typedef typename Persona::Value Value;
  typedef typename Persona::Metadata Metadata;
  typedef std::pair<Key, Value> KvPair;
  struct Contents;
  typedef std::map<NodeId, std::vector<Contents>> TransferInfo;

  struct Contents {
    GroupName group_name;
    Metadata metadata;
    std::vector<KvPair> kv_pair;
  };

  GroupDb();
  ~GroupDb();

  void AddGroup(const GroupName& group_name, const Metadata& metadata);
  // use only in case of leaving or unregister
  void DeleteGroup(const GroupName& group_name);
  // For atomically updating metadata only
  void Commit(const GroupName& group_name, std::function<void(Metadata& metadata)> functor);
  // For atomically updating metadata and value
  void Commit(const Key& key,
      std::function<detail::DbAction(Metadata& metadata, std::unique_ptr<Value>& value)> functor);
  TransferInfo GetTransferInfo(std::shared_ptr<routing::MatrixChange> matrix_change);
  void HandleTransfer(const std::vector<Contents>& contents);

  // returns metadata if group_name exists in db
  Metadata GetMetadata(const GroupName& group_name);
  // returns value if key exists in db
  Value GetValue(const Key& key);
  Contents GetContents(const GroupName& group_name);

 private:
  typedef uint32_t GroupId;
  typedef std::map<GroupName, std::pair<GroupId, Metadata>> GroupMap;
  GroupDb(const GroupDb&);
  GroupDb& operator=(const GroupDb&);
  GroupDb(GroupDb&&);
  GroupDb& operator=(GroupDb&&);

  void DeleteGroupEntries(const GroupName& group_name);
  Value Get(const Key& key, const GroupId& group_id);
  void Put(const KvPair& key_value_pair, const GroupId& group_id);
  void Delete(const Key& key, const GroupId& group_id);
  std::string MakeLevelDbKey(const GroupId& group_id, const Key& key);
  Key MakeKey(const GroupName group_name, const leveldb::Slice& level_db_key);
  uint32_t GetGroupId(const leveldb::Slice& level_db_key);
  typename GroupMap::iterator FindGroup(const GroupName& group_name);

  static const int kPrefixWidth_ = 2;
  const boost::filesystem::path kDbPath_;
  std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
  GroupMap group_map_;
};

template <typename Persona>
GroupDb<Persona>::GroupDb()
    : kDbPath_(boost::filesystem::unique_path()),
      mutex_(),
      leveldb_(InitialiseLevelDb(kDbPath_)),
      group_map_() {
  // Remove this assert if value needs to be copy constructible.
  // this is just a check to avoid copy constructor unless we require it
  static_assert(!std::is_copy_constructible<typename Persona::Value>::value,
                "value should not be copy constructible !");
  static_assert(std::is_move_constructible<typename Persona::Value>::value,
                "value should be move constructible !");
}

template <typename Persona>
GroupDb<Persona>::~GroupDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

template <typename Persona>
void GroupDb<Persona>::AddGroup(const GroupName& group_name, const Metadata& metadata) {
  std::lock_guard<std::mutex> lock(mutex_);
  static const uint64_t kGroupsLimit(static_cast<GroupId>(std::pow(256, kPrefixWidth_)));
  if (group_map_.size() == kGroupsLimit - 1)
    ThrowError(VaultErrors::failed_to_handle_request);
  GroupId group_id(RandomInt32() % kGroupsLimit);
  while (std::any_of(std::begin(group_map_), std::end(group_map_),
                     [&group_id](const std::pair<GroupName, std::pair<GroupId, Metadata>>&
                                 element) { return group_id == element.second.first; })) {
    group_id = RandomInt32() % kGroupsLimit;
  }
  LOG(kVerbose) << "GroupDb<Persona>::AddGroup size of group_map_ " << group_map_.size()
                << " current group_name " << HexSubstr(group_name->string());
  if (!(group_map_.insert(std::make_pair(group_name, std::make_pair(group_id, metadata)))).second) {
    LOG(kError) << "account already exists in the group map";
    ThrowError(VaultErrors::account_already_exists);
  }
  LOG(kInfo) << "group inserting succeeded for group_name "
             << HexSubstr(group_name->string());
}

template <typename Persona>
void GroupDb<Persona>::DeleteGroup(const GroupName& group_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  DeleteGroupEntries(group_name);
}

template <typename Persona>
void GroupDb<Persona>::Commit(const GroupName& group_name,
                              std::function<void(Metadata& metadata)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(FindGroup(group_name));
  functor(it->second.second);
}

template <typename Persona>
void GroupDb<Persona>::Commit(
    const Key& key,
    std::function<detail::DbAction(Metadata& metadata, std::unique_ptr<Value>& value)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(FindGroup(key.group_name()));
  std::unique_ptr<Value> value;
  try {
    value.reset(new Value(Get(key, it->second.first)));
  } catch (const common_error& error) {
    if (error.code().value() != static_cast<int>(CommonErrors::no_such_element))
      throw error;  // throw only for db errors
  }
  if (detail::DbAction::kPut == functor(it->second.second, value)) {
    Put(std::make_pair(key, std::move(*value)), it->second.first);
  } else {
    assert(value);
    Delete(key, it->second.first);
  }
}

template <typename Persona>
typename GroupDb<Persona>::Contents GroupDb<Persona>::GetContents(const GroupName& group_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(FindGroup(group_name));
  Contents contents;
  contents.group_name = it->first;
  contents.metadata = it->second.second;
  // get db entry
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto group_id = it->second.first;
  auto group_id_str = detail::ToFixedWidthString<kPrefixWidth_>(group_id);
  if (++it != group_map_.end()) {
    for (iter->Seek(group_id_str);
         (iter->Valid() && (GetGroupId(iter->key()) < group_id));
         iter->Next()) {
      contents.kv_pair.push_back(std::make_pair(MakeKey(contents.group_name, iter->key()),
                                                Value(iter->value().ToString())));
    }
  } else {
    for (iter->Seek(group_id_str); iter->Valid(); iter->Next()) {
      contents.kv_pair.push_back(std::make_pair(MakeKey(contents.group_name, iter->key()),
                                                Value(iter->value().ToString())));
    }
  }

  iter.reset();
  return contents;
}


// FIXME (Prakash)
template <typename Persona>
typename GroupDb<Persona>::TransferInfo GroupDb<Persona>::GetTransferInfo(
    std::shared_ptr<routing::MatrixChange> matrix_change) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<GroupName> prune_vector;
  TransferInfo transfer_info;
  for (const auto& group : group_map_) {
    auto check_holder_result = matrix_change->CheckHolders(NodeId(group.first->string()));
    if (check_holder_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      if (check_holder_result.new_holders.size() != 0) {
        assert(check_holder_result.new_holders.size() == 1);
        auto found_itr = transfer_info.find(check_holder_result.new_holders.at(0));
        if (found_itr != transfer_info.end()) {
          // Add to map
        } else {  // create contents
                  // Add to map
        }
      }
    } else {  // Prune group
      prune_vector.push_back(group);
    }
  }

  for (const auto& i : prune_vector)
    DeleteGroupEntries(i);
  return transfer_info;
}

// FIXME (Prakash)
// Ignores values which are already in db
template <typename Persona>
void GroupDb<Persona>::HandleTransfer(const std::vector<Contents>& contents) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& kv_pair : contents) {
  }
}

template <typename Persona>
typename GroupDb<Persona>::Metadata GroupDb<Persona>::GetMetadata(const GroupName& group_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(FindGroup(group_name));
  return it->second.second;
}


template <typename Persona>
typename GroupDb<Persona>::Value GroupDb<Persona>::GetValue(const Key& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(FindGroup(key.group_name()));
  return Get(key, it->second.first);
}

template <typename Persona>
void GroupDb<Persona>::DeleteGroupEntries(const GroupName& group_name) {
  std::vector<std::string> group_db_keys;
  typename GroupMap::iterator it;
  try {
    it = FindGroup(group_name);
  } catch (const vault_error& error) {
    LOG(kInfo) << "account doesn't exist for group " << DebugId(group_name);
    return;
  }
  auto group_id = it->second.first;
  auto group_id_str = detail::ToFixedWidthString<kPrefixWidth_>(group_id);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  if (++it != group_map_.end()) {
    for (iter->Seek(group_id_str);
         (iter->Valid() && (GetGroupId(iter->key()) < group_id));
         iter->Next())
      group_db_keys.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(group_id_str); iter->Valid(); iter->Next()) {
      group_db_keys.push_back(iter->key().ToString());
    }
  }

  iter.reset();

  for (const auto& key : group_db_keys) {
    leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), key));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }
  group_map_.erase(group_name);
  leveldb_->CompactRange(nullptr, nullptr);
}

// throws
template <typename Persona>
typename GroupDb<Persona>::Value GroupDb<Persona>::Get(const Key& key, const GroupId& group_id) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value_string;
  leveldb::Status status(
              leveldb_->Get(read_options, MakeLevelDbKey(group_id, key), &value_string));
  if (status.ok()) {
    assert(!value_string.empty());
    return Value(value_string);
  } else if (status.IsNotFound()) {
    ThrowError(CommonErrors::no_such_element);
  }
  ThrowError(VaultErrors::failed_to_handle_request);
  return Value();
}

template <typename Persona>
void GroupDb<Persona>::Put(const KvPair& key_value_pair, const GroupId& group_id) {
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       MakeLevelDbKey(group_id, key_value_pair.first),
                                       key_value_pair.second.Serialise()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template <typename Persona>
void GroupDb<Persona>::Delete(const Key& key, const GroupId& group_id) {
  leveldb::Status status(
      leveldb_->Delete(leveldb::WriteOptions(), MakeLevelDbKey(group_id, key)));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template <typename Persona>
std::string GroupDb<Persona>::MakeLevelDbKey(const GroupId& group_id, const Key& key) {
    return detail::ToFixedWidthString<kPrefixWidth_>(group_id) + key.ToFixedWidthString().string();
}

template <typename Persona>
typename Persona::Key GroupDb<Persona>::MakeKey(const GroupName group_name,
                                                const leveldb::Slice& level_db_key) {
  return Key(group_name, typename Persona::Key::FixedWidthString(
                 (level_db_key.ToString()).substr(kPrefixWidth_)));
}

template <typename Persona>
uint32_t GroupDb<Persona>::GetGroupId(const leveldb::Slice& level_db_key) {
  return detail::FromFixedWidthString<kPrefixWidth_>(
      (level_db_key.ToString()).substr(0, kPrefixWidth_));
}

// throws
template <typename Persona>
typename GroupDb<Persona>::GroupMap::iterator GroupDb<Persona>::FindGroup(
    const GroupName& group_name) {
  auto it(group_map_.find(group_name));
  if (it == group_map_.end())
    ThrowError(VaultErrors::no_such_account);
  return it;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_DB_H_
