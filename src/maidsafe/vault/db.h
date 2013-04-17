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
#include <set>

#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

class Db {
 public:
  typedef std::pair<DataNameVariant, NonEmptyString> KVPair;
  // throws on failure
  explicit Db(const boost::filesystem::path& path);

  ~Db();

  void Put(const KVPair& key_value_pair);
  void Delete(const DataNameVariant& key);
  NonEmptyString Get(const DataNameVariant& key);

  std::vector<KVPair> Get();

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);

  template<uint32_t Width> std::string Pad(uint32_t number);

  static std::mutex mutex_;
  static std::once_flag flag_;
  static std::unique_ptr<leveldb::DB> leveldb_;
  static std::atomic<uint32_t> last_account_id_; //FIXME
  static const uint32_t kPrefixWidth_, kSuffixWidth_;
  static std::set<uint32_t> account_ids_;
  uint32_t account_id_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
