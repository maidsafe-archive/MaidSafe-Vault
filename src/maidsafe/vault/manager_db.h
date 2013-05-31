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

#include <string>
#include <utility>


#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {
namespace vault {
template<typename PersonaType>
class ManagerDb {
 public:
  typedef std::pair<typename PersonaType::DbKey ,typename PersonaType::DbValue> KvPair;
  explicit ManagerDb(const boost::filesystem::path& path);
  ~ManagerDb();

  NonEmptyString Get(const typename PersonaType::DbKey& key);
  void Put(const KvPair& key_value_pair);
  void Delete(const typename PersonaType::DbKey& key);
  std::vector<typename PersonaType::DbKey> GetKeys();

 private:
  ManagerDb(const ManagerDb&);
  ManagerDb& operator=(const ManagerDb&);
  ManagerDb(ManagerDb&&);
  ManagerDb& operator=(ManagerDb&&);

  template<uint32_t Width> std::string Pad(uint32_t number);

  static const uint32_t kSuffixWidth_;
  const boost::filesystem::path kDbPath_;
  mutable std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MANAGER_DB_H_
