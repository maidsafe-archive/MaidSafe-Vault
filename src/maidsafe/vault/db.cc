/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/


#include "maidsafe/vault/db.h"

#include <iomanip>
#include <sstream>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/utils.h"

namespace maidsafe {
namespace vault {

const int Db::kPrefixWidth_(2);
const int Db::kSuffixWidth_(1);

Db::Db(const boost::filesystem::path& path)
    : kDbPath_(path),
      mutex_(),
      leveldb_(),
      account_ids_() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (boost::filesystem::exists(kDbPath_))
    boost::filesystem::remove_all(kDbPath_);
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, kDbPath_.string(), &db));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request); // FIXME need new exception
  leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
  assert(leveldb_);
}

Db::~Db() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

uint32_t Db::RegisterAccount() {
  std::lock_guard<std::mutex> lock(mutex_);
  static_assert(kPrefixWidth_ < 5,
                "If kPrefixWidth_ > 4, uint32_t is not suitable as the return type.");
  static const uint64_t kAccountsLimit(static_cast<uint32_t>(std::pow(256, kPrefixWidth_)));
  if (account_ids_.size() == kAccountsLimit - 1)
    ThrowError(VaultErrors::failed_to_handle_request);
  uint32_t account_id(RandomUint32() % kAccountsLimit);
  while (account_ids_.find(account_id) != account_ids_.end())
    account_id = RandomUint32() % kAccountsLimit;
  account_ids_.insert(account_id);
  return account_id;
}

void Db::UnRegisterAccount(const uint32_t& account_id) {
  std::vector<std::string> account_keys;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto it(account_ids_.find(account_id));
  if (it == account_ids_.end())
    return;

  if (++it != account_ids_.end()) {
    for (iter->Seek(detail::Pad<kPrefixWidth_>(account_id));
         iter->Valid() && iter->key().ToString() < detail::Pad<kPrefixWidth_>(*it);
         iter->Next())
      account_keys.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(detail::Pad<kPrefixWidth_>(account_id)); iter->Valid(); iter->Next())
      account_keys.push_back(iter->key().ToString());
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

void Db::Put(const uint32_t& account_id, const KvPair& key_value_pair) {
  std::string db_key(GetSerialisedKey(account_id, key_value_pair.first));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       db_key, key_value_pair.second.string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void Db::Delete(const uint32_t& account_id, const DataNameVariant& key) {
  std::string db_key(GetSerialisedKey(account_id, key));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

NonEmptyString Db::Get(const uint32_t& account_id, const DataNameVariant& key) {
  std::string db_key(GetSerialisedKey(account_id, key));
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, db_key, &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return NonEmptyString(value);
}

std::vector<Db::KvPair> Db::Get(const uint32_t& account_id) {
  std::vector<KvPair> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  auto it(account_ids_.find(account_id));
  assert(it != account_ids_.end());
  for (iter->Seek(detail::Pad<kPrefixWidth_>(account_id));
       iter->Valid() && ((++it == account_ids_.end()) ||
                         (iter->key().ToString() <detail::Pad<kPrefixWidth_>(*it)));
       iter->Next()) {
    std::string name(iter->key().ToString().substr(kPrefixWidth_, NodeId::kSize));
    std::string type_string(iter->key().ToString().substr(kPrefixWidth_ + NodeId::kSize));
    DataTagValue type = static_cast<DataTagValue>(std::stoul(type_string));
    auto key = GetDataNameVariant(type, Identity(name));
    KvPair kv_pair(key, NonEmptyString(iter->value().ToString()));
    return_vector.push_back(std::move(kv_pair));
  }
  return return_vector;
}

std::string Db::GetSerialisedKey(const uint32_t& account_id, const DataNameVariant& key) const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, key));
  return std::string(detail::Pad<kPrefixWidth_>(account_id) +
                     result.second.string() +
                     detail::Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
}

}  // namespace vault
}  // namespace maidsafe
