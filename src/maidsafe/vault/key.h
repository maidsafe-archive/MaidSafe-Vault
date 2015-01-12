/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_KEY_H_
#define MAIDSAFE_VAULT_KEY_H_

#include <string>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_type_values.h"
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/vault/key_utils.h"

namespace maidsafe {

namespace vault {

namespace test {
class KeyTest_BEH_Serialise_Test;
class KeyTest_BEH_All_Test;
}  // namespace test

template <typename Key, typename Value>
class Db;

struct Key {
  Key();
  Key(const Identity& name_in, DataTagValue type_in);
  explicit Key(const DataNameVariant& data_name);
  explicit Key(const std::string& serialised_key);
  template <typename DataNameType>
  explicit Key(const DataNameType& data_name)
      : name(data_name.value), type(DataNameType::data_type::Tag::kValue) {}
  Key(const Key& other);
  Key(Key&& other) MAIDSAFE_NOEXCEPT;
  Key& operator=(Key other);
  std::string Serialise() const;

  Identity name;
  DataTagValue type;

  template <typename Key, typename Value>
  friend class Db;
  friend class DataManagerDataBase;

 private:
  typedef maidsafe::detail::BoundedString<NodeId::kSize + detail::PaddedWidth::value,
                                          NodeId::kSize + detail::PaddedWidth::value>
      FixedWidthString;

  explicit Key(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};

void swap(Key& lhs, Key& rhs) MAIDSAFE_NOEXCEPT;
bool operator==(const Key& lhs, const Key& rhs);
bool operator!=(const Key& lhs, const Key& rhs);
bool operator<(const Key& lhs, const Key& rhs);
bool operator>(const Key& lhs, const Key& rhs);
bool operator<=(const Key& lhs, const Key& rhs);
bool operator>=(const Key& lhs, const Key& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_H_
