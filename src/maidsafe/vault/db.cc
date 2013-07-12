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

#include "maidsafe/vault/db.h"

#include "leveldb/status.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

const int Db::kPrefixWidth_(2);

Db::Db()
    : kDbPath_(boost::filesystem::unique_path()),
      mutex_(),
      leveldb_(InitialiseLevelDb(kDbPath_)),
      account_ids_() {}

Db::~Db() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

Db::AccountId Db::RegisterAccount() {
  std::lock_guard<std::mutex> lock(mutex_);
  static_assert(kPrefixWidth_ < 5,
                "If kPrefixWidth_ > 4, uint32_t is not suitable as the return type.");
  static const uint64_t kAccountsLimit(static_cast<AccountId>(std::pow(256, kPrefixWidth_)));
  if (account_ids_.size() == kAccountsLimit - 1)
    ThrowError(VaultErrors::failed_to_handle_request);
  AccountId account_id(RandomInt32() % kAccountsLimit);
  while (account_ids_.find(account_id) != account_ids_.end())
    account_id = RandomInt32() % kAccountsLimit;
  account_ids_.insert(account_id);
  return account_id;
}

void Db::UnRegisterAccount(const AccountId& account_id) {
  std::vector<std::string> account_keys;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto it(account_ids_.find(account_id));
  if (it == account_ids_.end())
    return;

  if (++it != account_ids_.end()) {
    for (iter->Seek(detail::ToFixedWidthString<kPrefixWidth_>(account_id));
         iter->Valid() && iter->key().ToString() < detail::ToFixedWidthString<kPrefixWidth_>(*it);
         iter->Next())
      account_keys.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(detail::ToFixedWidthString<kPrefixWidth_>(account_id)); iter->Valid();
         iter->Next()) {
      account_keys.push_back(iter->key().ToString());
    }
  }

  iter.reset();

  for (auto i: account_keys) {
    leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), i));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }
  account_ids_.erase(account_id);
  leveldb_->CompactRange(nullptr, nullptr);
}

void Db::Put(const AccountId& account_id, const KvPair& key_value_pair) {
  std::string db_key(SerialiseKey(account_id, key_value_pair.first));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       db_key, key_value_pair.second.string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void Db::Delete(const AccountId& account_id, const KvPair::first_type& key) {
  std::string db_key(SerialiseKey(account_id, key));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

NonEmptyString Db::Get(const AccountId& account_id, const KvPair::first_type& key) {
  std::string db_key(SerialiseKey(account_id, key));
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, db_key, &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return NonEmptyString(value);
}

std::vector<Db::KvPair> Db::Get(const AccountId& account_id) {
  std::vector<KvPair> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto it(account_ids_.find(account_id));
  assert(it != account_ids_.end());
  for (iter->Seek(detail::ToFixedWidthString<kPrefixWidth_>(account_id));
       iter->Valid() && ((++it == account_ids_.end()) ||
                         (iter->key().ToString() < detail::ToFixedWidthString<kPrefixWidth_>(*it)));
       iter->Next()) {
    auto account_id_and_key(ParseKey(iter->key().ToString()));
    return_vector.push_back(std::make_pair(account_id_and_key.second,
                                           NonEmptyString(iter->value().ToString())));
  }
  return return_vector;
}

std::string Db::SerialiseKey(const AccountId& account_id, const KvPair::first_type& key) const {
  return detail::ToFixedWidthString<kPrefixWidth_>(account_id) + key.Serialise();
}

std::pair<Db::AccountId, Db::KvPair::first_type> Db::ParseKey(
    const std::string& serialised_key) const {
  std::string account_id_as_string(serialised_key.substr(0, kPrefixWidth_));
  return std::make_pair(detail::FromFixedWidthString<kPrefixWidth_>(account_id_as_string),
                        DbKey(serialised_key.substr(kPrefixWidth_)));
}

}  // namespace vault

}  // namespace maidsafe
