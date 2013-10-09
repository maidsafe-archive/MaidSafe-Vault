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
//#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

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
      std::function<detail::DbAction(Metadata& metadata, boost::optional<Value>& value)> functor);
  TransferInfo GetTransferInfo(std::shared_ptr<routing::MatrixChange> matrix_change);
  void HandleTransfer(const std::vector<Contents>& contents);

  // returns metadata if group_name exists in db
  boost::optional<Metadata> GetMetadata(const GroupName& group_name);
  // returns value if key exists in db
  boost::optional<Value> GetValue(const Key& key);
  boost::optional<Contents> GetContents(const GroupName& group_name);

 private:
  typedef uint32_t GroupId;
  GroupDb(const GroupDb&);
  GroupDb& operator=(const GroupDb&);
  GroupDb(GroupDb&&);
  GroupDb& operator=(GroupDb&&);

  boost::optional<Metadata> Get(const GroupName& group_name);
  boost::optional<Value> Get(const Key& key);
  void DeleteGroupEntries(const GroupName& group_name);
  void PutValue(const KvPair& key_value);
  void DeleteValue(const Key& key);

  static const int kPrefixWidth_ = 2;
  const boost::filesystem::path kDbPath_;
  std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
  std::map<GroupName, std::pair<GroupId, Metadata>> group_map_;
};

template <typename Persona>
GroupDb<Persona>::GroupDb()
    : kDbPath_(boost::filesystem::unique_path()),
      mutex_(),
      leveldb_(InitialiseLevelDb(kDbPath_)),
      group_map_() {}

template <typename Persona>
GroupDb<Persona>::~GroupDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

template <typename Persona>
boost::optional<typename GroupDb<Persona>::Contents> GroupDb<Persona>::GetContents(const GroupName& /*group_name*/) {
  return boost::optional<GroupDb<Persona>::Contents>();
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
  if (!(group_map_.insert(std::make_pair(group_name, std::make_pair(group_id, metadata)))).second)
    ThrowError(VaultErrors::failed_to_handle_request);  // TODO change to account already exist!
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
  auto it(group_map_.find(group_name));
  if (it == group_map_.end())
    ThrowError(CommonErrors::no_such_element);
  functor(it->second.second);
}

template <typename Persona>
void GroupDb<Persona>::Commit(
    const Key& key,
    std::function<detail::DbAction(Metadata& metadata, boost::optional<Value>& value)> functor) {
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  auto it(group_map_.find(key.group_name()));
  if (it == group_map_.end())
    ThrowError(CommonErrors::no_such_element);

  boost::optional<Value> value(GetValue(key));
  if (detail::DbAction::kPut == functor(it->second.second, value)) {
    PutValue(std::make_pair(key, *value));
  } else {
    assert(value);
    DeleteValue(key);
  }
}

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

// Ignores values which are already in db
template <typename Persona>
void GroupDb<Persona>::HandleTransfer(const std::vector<Contents>& contents) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (const auto& kv_pair : contents) {
  }
}

// throws
template <typename Persona>
boost::optional<typename GroupDb<Persona>::Metadata> GroupDb<Persona>::Get(
    const GroupName& group_name) {
  boost::optional<Metadata> return_value;
  auto it(group_map_.find(group_name));
  if (it != group_map_.end())
    return_value = it->second.second;
  return return_value;
}

template <typename Persona>
boost::optional<typename GroupDb<Persona>::Metadata> GroupDb<Persona>::GetMetadata(
    const GroupName& group_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  return Get(group_name);
}

// throws
template <typename Persona>
boost::optional<typename GroupDb<Persona>::Value> GroupDb<Persona>::Get(const Key& key) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value_string;
  leveldb::Status status(
      leveldb_->Get(read_options, key.ToFixedWidthString().string(), &value_string));
  if (status.ok()) {
    assert(!value_string.empty());
    return boost::optional<Value>(value_string);
  } else if (status.IsNotFound()) {
    return boost::optional<Value>();
  }
  ThrowError(VaultErrors::failed_to_handle_request);
  return boost::optional<Value>();
}

// not locking here, as leveldb is thread safe.
template <typename Persona>
boost::optional<typename GroupDb<Persona>::Value> GroupDb<Persona>::GetValue(const Key& key) {
  return Get(key);
}

template <typename Persona>
void GroupDb<Persona>::DeleteGroupEntries(const GroupName& group_name) {
  std::vector<std::string> group_db_keys;
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto it(group_map_.find(group_name));
  if (it == group_map_.end())
    return;
  auto group_id = it->second.first;
  auto group_id_str = detail::ToFixedWidthString<kPrefixWidth_>(group_id);
  if (++it != group_map_.end()) {
    for (iter->Seek(detail::ToFixedWidthString<kPrefixWidth_>(group_id));
         iter->Valid() && iter->key().ToString() < group_id_str; iter->Next())
      group_db_keys.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(detail::ToFixedWidthString<kPrefixWidth_>(group_id)); iter->Valid();
         iter->Next()) {
      group_db_keys.push_back(iter->key().ToString());
    }
  }

  iter.reset();

  for (auto i : group_db_keys) {
    leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), i));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }
  group_map_.erase(group_name);
  leveldb_->CompactRange(nullptr, nullptr);
}

template <typename Persona>
void GroupDb<Persona>::PutValue(const KvPair& /*key_value*/) {}

template <typename Persona>
void GroupDb<Persona>::DeleteValue(const Key& /*key*/) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_DB_H_
