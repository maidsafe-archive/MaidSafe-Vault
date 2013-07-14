/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_DB_DataManagerKey_H_
#define MAIDSAFE_VAULT_DB_DataManagerKey_H_

#include <string>

#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

namespace test {
class DbDataManagerKeyTest_BEH_Serialise_Test;
class DbDataManagerKeyTest_BEH_All_Test;
}  // namespace test

class Db;
template<typename DataManagerKey, typename Value>
class ManagerDb;

class DataManagerKey {
 public:
  explicit DataManagerKey(const DataNameVariant& name);
  DataManagerKey();
  DataManagerKey(const DataManagerKey& other);
  DataManagerKey(DataManagerKey&& other);
  DataManagerKey& operator=(DataManagerKey other);

  DataNameVariant name() const { return name_; }

  friend void swap(DataManagerKey& lhs, DataManagerKey& rhs) MAIDSAFE_NOEXCEPT;
  friend bool operator==(const DataManagerKey& lhs, const DataManagerKey& rhs);
  friend bool operator<(const DataManagerKey& lhs, const DataManagerKey& rhs);
  friend class Db;
  friend class test::DbDataManagerKeyTest_BEH_Serialise_Test;
  friend class test::DbDataManagerKeyTest_BEH_All_Test;

 private:
  explicit DataManagerKey(const std::string& serialised_db_DataManagerKey);
  std::string Serialise() const;
  DataNameVariant name_;
  static const int kPaddedWidth_;
};

bool operator!=(const DataManagerKey& lhs, const DataManagerKey& rhs);

bool operator>(const DataManagerKey& lhs, const DataManagerKey& rhs);

bool operator<=(const DataManagerKey& lhs, const DataManagerKey& rhs);

bool operator>=(const DataManagerKey& lhs, const DataManagerKey& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_DataManagerKey_H_
