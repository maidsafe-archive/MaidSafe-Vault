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

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/db_key.h"


namespace maidsafe {

namespace vault {

class AccountDb;

class Db {
 public:
  typedef std::pair<DbKey, NonEmptyString> KvPair;
  typedef uint32_t AccountId;

  explicit Db(const boost::filesystem::path& path);
  ~Db();
  friend class AccountDb;

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);

  AccountId RegisterAccount();
  void UnRegisterAccount(const AccountId& account_id);

  void Put(const AccountId& account_id, const KvPair& key_value_pair);
  void Delete(const AccountId& account_id, const KvPair::first_type& key);
  NonEmptyString Get(const AccountId& account_id, const KvPair::first_type& key);
  std::vector<KvPair> Get(const AccountId& account_id);

  std::string SerialiseKey(const AccountId& account_id, const KvPair::first_type& key) const;
  std::pair<AccountId, KvPair::first_type> ParseKey(const std::string& serialised_key) const;

  static const int kPrefixWidth_;
  const boost::filesystem::path kDbPath_;
  std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
  std::set<AccountId> account_ids_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
