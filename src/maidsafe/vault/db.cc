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

#include <sstream>
#include <iomanip>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {
namespace vault {

std::mutex Db::mutex_;
std::unique_ptr<leveldb::DB> Db::leveldb_ = nullptr;
std::atomic<uint32_t> Db::last_account_id_(0);
std::once_flag Db::flag_;
const uint32_t Db::kPrefixWidth_(4);
const uint32_t Db::kSuffixWidth_(2);
std::set<uint32_t> Db::account_ids_;

Db::Db(const boost::filesystem::path& path)
  : db_path_(path),
    account_id_(0) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!leveldb_) {
      if (boost::filesystem::exists(path))
        boost::filesystem::remove_all(path);
      leveldb::DB* db;
      leveldb::Options options;
      options.create_if_missing = true;
      options.error_if_exists = true;
      leveldb::Status status(leveldb::DB::Open(options, path.string(), &db));
      if (!status.ok())
        ThrowError(VaultErrors::failed_to_handle_request); // FIXME need new exception
      leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
      assert(leveldb_);
  }
  if (account_ids_.size() == (1 << 16) - 1)
    ThrowError(VaultErrors::failed_to_handle_request);
  uint32_t account_id(RandomUint32() % (1 << 16));
  while (account_ids_.find((account_id)) != account_ids_.end())
    account_id = RandomUint32() % (1 << 16);
  account_ids_.insert(account_id);
  account_id_ = account_id;
}

Db::~Db() {
  std::vector<std::string> account_elements;
  std::lock_guard<std::mutex> lock(mutex_);
  leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
  auto it(account_ids_.find(account_id_));
  assert(it != account_ids_.end());
  if (++it != account_ids_.end()) {
    for (iter->Seek(Pad<kPrefixWidth_>(account_id_));
         iter->Valid() && iter->key().ToString() < Pad<kPrefixWidth_>(*it);
         iter->Next())
      account_elements.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(Pad<kPrefixWidth_>(account_id_)); iter->Valid(); iter->Next())
      account_elements.push_back(iter->key().ToString());
  }
  delete iter;
  account_ids_.erase(account_id_);
  if (account_ids_.size() == 0) {
    leveldb::DestroyDB(db_path_.string(), leveldb::Options());
    leveldb_.reset();
    return;
  }

  for (auto i: account_elements) {
    leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), i));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }
}

void Db::Put(const KVPair& key_value_pair) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key_value_pair.first));
  std::string db_key(Pad<kPrefixWidth_>(account_id_) +
                     result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       db_key, key_value_pair.second.string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void Db::Delete(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(Pad<kPrefixWidth_>(account_id_) +
                     result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

NonEmptyString Db::Get(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(Pad<kPrefixWidth_>(account_id_) +
                     result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, db_key, &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return NonEmptyString(value);
}

std::vector<Db::KVPair> Db::Get() {
  std::vector<KVPair> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
  auto it(account_ids_.find(account_id_));
  assert(it != account_ids_.end());
  for (iter->Seek(Pad<kPrefixWidth_>(account_id_));
       iter->Valid() && ((++it == account_ids_.end()) ||
                         (iter->key().ToString() < Pad<kPrefixWidth_>(*it)));
       iter->Next()) {
    std::string name(iter->key().ToString().substr(kPrefixWidth_, NodeId::kSize));
    std::string type_string(iter->key().ToString().substr(kPrefixWidth_ + NodeId::kSize));
    DataTagValue type = static_cast<DataTagValue>(std::stoul(type_string));
    auto key = GetDataNameVariant(type, Identity(name));
    KVPair kv_pair(key, NonEmptyString(iter->value().ToString()));
    return_vector.push_back(std::move(kv_pair));
  }
  delete iter;
  return return_vector;
}

template<uint32_t Width>
std::string Db::Pad(uint32_t number)
{
    std::ostringstream osstream;
    osstream << std::setw(Width) << std::setfill('0') << std::hex << number;
    return osstream.str();
}

}  // namespace vault
}  // namespace maidsafe
