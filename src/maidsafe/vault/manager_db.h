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

#ifndef MAIDSAFE_VAULT_MANAGER_DB_H_
#define MAIDSAFE_VAULT_MANAGER_DB_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "leveldb/db.h"


namespace maidsafe {

namespace vault {

template<typename PersonaType>
class ManagerDb {
 public:
  typedef std::pair<typename PersonaType::DbKey, typename PersonaType::DbValue> KvPair;
  explicit ManagerDb(const boost::filesystem::path& path);
  ~ManagerDb();

  void Put(const KvPair& key_value_pair);
  void Delete(const typename PersonaType::DbKey& key);
  typename PersonaType::DbValue Get(const typename PersonaType::DbKey& key);
  std::vector<typename PersonaType::DbKey> GetKeys();

 private:
  ManagerDb(const ManagerDb&);
  ManagerDb& operator=(const ManagerDb&);
  ManagerDb(ManagerDb&&);
  ManagerDb& operator=(ManagerDb&&);

  const boost::filesystem::path kDbPath_;
  mutable std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/manager_db-inl.h"

#endif  // MAIDSAFE_VAULT_MANAGER_DB_H_
