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


#include "maidsafe/vault/metadata_manager/metadata_db.h"

#include <iomanip>
#include <sstream>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"

namespace maidsafe {
namespace vault {

const uint32_t MetadataDb::kSuffixWidth_(2);

MetadataDb::MetadataDb(const boost::filesystem::path& path)
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

MetadataDb::~MetadataDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

void MetadataDb::Put(const KvPair& key_value_pair) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key_value_pair.first));
  std::string db_key(result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       db_key, key_value_pair.second.string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

void MetadataDb::Delete(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(result.second.string() +
                     Pad<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), db_key));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

NonEmptyString MetadataDb::Get(const DataNameVariant& key) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key));
  std::string db_key(result.second.string() +
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

template<uint32_t Width>
std::string MetadataDb::Pad(uint32_t number) {
  std::ostringstream osstream;
  osstream << std::setw(Width) << std::setfill('0') << std::hex << number;
  return osstream.str();
}

}  // namespace vault
}  // namespace maidsafe
