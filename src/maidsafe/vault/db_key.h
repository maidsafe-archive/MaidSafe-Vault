/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
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

#include <string>

#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

namespace test {
  class DbKeyTest_BEH_Serialise_Test;
  class DbKeyTest_BEH_All_Test;
}

class Db;
template<typename PersonaType>
class ManagerDb;

class DbKey {
 public:
  explicit DbKey(const DataNameVariant& name);
  DbKey();
  DbKey(const DbKey& other);
  DbKey(DbKey&& other);
  DbKey& operator=(DbKey other);

  DataNameVariant name() const { return name_; }

  friend void swap(DbKey& lhs, DbKey& rhs) MAIDSAFE_NOEXCEPT;
  friend bool operator==(const DbKey& lhs, const DbKey& rhs);
  friend bool operator<(const DbKey& lhs, const DbKey& rhs);
  friend class Db;
  template<typename PersonaType>
  friend class ManagerDb;
  friend class test::DbKeyTest_BEH_Serialise_Test;
  friend class test::DbKeyTest_BEH_All_Test;

 private:
  explicit DbKey(const std::string& serialised_db_key);
  std::string Serialise() const;
  DataNameVariant name_;
  static const int kPaddedWidth_;
};

bool operator!=(const DbKey& lhs, const DbKey& rhs);

bool operator>(const DbKey& lhs, const DbKey& rhs);

bool operator<=(const DbKey& lhs, const DbKey& rhs);

bool operator>=(const DbKey& lhs, const DbKey& rhs);


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_KEY_H_
