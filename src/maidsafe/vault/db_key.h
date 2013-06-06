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

#ifndef MAIDSAFE_VAULT_DB_KEY_H_
#define MAIDSAFE_VAULT_DB_KEY_H_

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {

namespace vault {

class Db;
class ManagerDb;

class DbKey {
  explicit DbKey(const DataNameVariant& name);
  DbKey(const DbKey& other);
  DbKey(DbKey&& other);
  DbKey& operator=(DbKey other);
  std::string Serialise() const;
 private:
  friend class ManagerDb;
  friend class Db;
  explicit DbKey(const std::string& serialised_db_key);
  DataNameVariant name_;
  static const int kPaddedWidth;
};


void swap(DbKey& lhs, DbKey& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const DbKey& lhs, const DbKey& rhs);

bool operator!=(const DbKey& lhs, const DbKey& rhs);

bool operator<(const DbKey& lhs, const DbKey& rhs);

bool operator>(const DbKey& lhs, const DbKey& rhs);

bool operator<=(const DbKey& lhs, const DbKey& rhs);

bool operator>=(const DbKey& lhs, const DbKey& rhs);


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_KEY_H_
