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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_DB_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_DB_H_

#include <string>
#include <utility>


#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {
namespace vault {

class MetadataDb {
 public:
  typedef std::pair<DataNameVariant, NonEmptyString> KvPair;
  explicit MetadataDb(const boost::filesystem::path& path);
  ~MetadataDb();

  NonEmptyString Get(const DataNameVariant& key);
  void Put(const KvPair& key_value_pair);
  void Delete(const DataNameVariant& key);

 private:
  MetadataDb(const MetadataDb&);
  MetadataDb& operator=(const MetadataDb&);
  MetadataDb(MetadataDb&&);
  MetadataDb& operator=(MetadataDb&&);

  template<uint32_t Width> std::string Pad(uint32_t number);

  static const uint32_t kSuffixWidth_;
  const boost::filesystem::path kDbPath_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_DB_H_
