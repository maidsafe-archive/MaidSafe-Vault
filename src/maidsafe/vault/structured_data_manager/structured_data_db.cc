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


#include "maidsafe/vault/structured_data_manager/structured_data_db.h"

#include <iomanip>
#include <sstream>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"

namespace maidsafe {
namespace vault {

const uint32_t StructuredDataDb::kSuffixWidth_(2);

StructuredDataDb::StructuredDataDb(const boost::filesystem::path& path)
  : kDbPath_(path),
    leveldb_() {
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

StructuredDataDb::~StructuredDataDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

void StructuredDataDb::Put(const KvPair& key_value_pair) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key_value_pair.first.first));
  std::string db_key(result.second.string() +
                   Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)) +
                   key_value_pair.first.second.node_id.string() +
                   Pad<kSuffixWidth_>(static_cast<uint32_t>(key_value_pair.first.second.persona)));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       db_key, NonEmptyString(key_value_pair.second).string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void StructuredDataDb::Delete(const Key &key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
  std::string db_key(result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)) +
                     key.second.node_id.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(key.second.persona)));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

StructuredDataVersions::serialised_type StructuredDataDb::Get(const Key &key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
  std::string db_key(result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)) +
                     key.second.node_id.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(key.second.persona)));
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, db_key, &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return StructuredDataVersions::serialised_type(NonEmptyString(value));
}

template<uint32_t Width>
std::string StructuredDataDb::Pad(uint32_t number) {
  std::ostringstream osstream;
  osstream << std::setw(Width) << std::setfill('0') << std::hex << number;
  return osstream.str();
}

}  // namespace vault
}  // namespace maidsafe
