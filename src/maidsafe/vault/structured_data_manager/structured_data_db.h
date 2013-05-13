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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_DB_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_DB_H_

#include <string>
#include <utility>


#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {
namespace vault {

class StructuredDataDb {
 public:
  typedef std::pair<DataNameVariant, Identity> Key;
  typedef Identity Value;
  typedef std::pair<Key, Value> KvPair;
  explicit StructuredDataDb(const boost::filesystem::path& path);
  ~StructuredDataDb();

  NonEmptyString Get(const Key& key);
  void Put(const KvPair& key_value_pair);
  void Delete(const Key& key);

 private:
  StructuredDataDb(const StructuredDataDb&);
  StructuredDataDb& operator=(const StructuredDataDb&);
  StructuredDataDb(StructuredDataDb&&);
  StructuredDataDb& operator=(StructuredDataDb&&);

  template<uint32_t Width> std::string Pad(uint32_t number);

  static const uint32_t kSuffixWidth_;
  const boost::filesystem::path kDbPath_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_DB_H_
