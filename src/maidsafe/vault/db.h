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

#ifndef MAIDSAFE_VAULT_DB_H_
#define MAIDSAFE_VAULT_DB_H_

#include <atomic>
#include <string>

#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

class Db {
 public:
  // throws on failure
  explicit Db(const boost::filesystem::path& path);

  ~Db(); // delete account here

  void Put(const DataNameVariant& key, const NonEmptyString& value);

  void Delete(const DataNameVariant& key);

  NonEmptyString Get(const DataNameVariant& key);

  std::vector<std::pair<std::string, std::string>> Get();

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);

  static std::once_flag flag;
  static std::unique_ptr<leveldb::DB> leveldb_;
  static std::atomic<uint32_t> last_account_id_;
  uint32_t account_id_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
