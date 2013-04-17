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

std::unique_ptr<leveldb::DB> Db::leveldb_ = nullptr;
std::atomic<uint32_t> Db::last_account_id_(0);
std::once_flag Db::flag;

Db::Db(const boost::filesystem::path& path) {
  std::call_once(flag, [&path](){
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
    });
  account_id_ = ++last_account_id_;
}

//FIXME need mutex here
Db::~Db() {
  std::vector<std::string> account_elements;
  leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
  if (account_id_ != last_account_id_) {
    for (iter->Seek(Pad<4>(account_id_));
         iter->Valid() && iter->key().ToString() < Pad<4>(account_id_ + 1);
         iter->Next())
      account_elements.push_back(iter->key().ToString());
  } else {
    for (iter->Seek(Pad<4>(account_id_)); iter->Valid(); iter->Next())
      account_elements.push_back(iter->key().ToString());
  }
  delete iter;

  for (auto i: account_elements) {
    leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), i));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }
}

void Db::Put(const DataNameVariant& key, const NonEmptyString& value) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(Pad<4>(account_id_) +
                     result.second.string() +
                     Pad<2>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(), db_key, value.string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void Db::Delete(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(Pad<4>(account_id_) +
                     result.second.string() +
                     Pad<2>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

NonEmptyString Db::Get(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(Pad<4>(account_id_) +
                     result.second.string() +
                     Pad<2>(static_cast<uint32_t>(result.first)));
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, db_key, &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return NonEmptyString(value);
}

//FIXME need mutex here
// Do we need to return key string or key string postfixed with type?
std::vector<std::pair<std::string, std::string>> Db::Get() {
  std::vector<std::pair<std::string, std::string>> return_vector;
  leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
  if (account_id_ != last_account_id_) {
    for (iter->Seek(Pad<4>(account_id_));
         iter->Valid() && iter->key().ToString() < Pad<4>(account_id_ + 1);
         iter->Next()) {
      std::pair<std::string, std::string> kv_pair(iter->key().ToString().substr(4),
                                                  iter->value().ToString());
      return_vector.push_back(std::move(kv_pair));
    }
  } else {
    for (iter->Seek(Pad<4>(account_id_)); iter->Valid(); iter->Next()) {
      std::pair<std::string, std::string> kv_pair(iter->key().ToString().substr(4),
                                                  iter->value().ToString());
      return_vector.push_back(std::move(kv_pair));
    }
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
