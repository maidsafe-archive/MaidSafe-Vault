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
#include <set>
#include <string>
#include <utility>
#include <vector>


#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {
namespace vault {

class AccountDb;

class Db {
 public:
  typedef std::pair<DataNameVariant, NonEmptyString> KvPair;

  explicit Db(const boost::filesystem::path& path);
  ~Db();
  friend class AccountDb;
 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);

  uint32_t RegisterAccount();
  void UnRegisterAccount(const uint32_t& account_id);

  void Put(const uint32_t& account_id, const KvPair& key_value_pair);
  void Delete(const uint32_t& account_id, const DataNameVariant& key);
  NonEmptyString Get(const uint32_t& account_id, const DataNameVariant& key);
  std::vector<KvPair> Get(const uint32_t& account_id);

  template<uint32_t Width> std::string Pad(uint32_t number);

  static const uint32_t kPrefixWidth_, kSuffixWidth_;
  const boost::filesystem::path kDbPath_;
  std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
  std::set<uint32_t> account_ids_;
};

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
