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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_KEY_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_KEY_H_

#include <string>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/key_utils.h"

namespace maidsafe {

namespace vault {

struct DataManagerKey {
  DataManagerKey();
  template <typename DataNameType>
  DataManagerKey(const DataNameType& name_in, const Identity& originator_in)
      : name(name_in.value), type(DataNameType::data_type::Tag::kValue),
        originator(originator_in) {}
  DataManagerKey(const Identity& name_in, const DataTagValue type_in,
                 const Identity& originator_in);
  explicit DataManagerKey(const std::string& serialised_key);
  DataManagerKey(const DataManagerKey& other);
  DataManagerKey(DataManagerKey&& other);
  DataManagerKey& operator=(DataManagerKey other);
  std::string Serialise() const;
  void CleanUpOriginator() { originator = Identity(NodeId().string()); }

  Identity name;
  DataTagValue type;
  Identity originator;

  template <typename Key, typename Value>
  friend class Db;

 private:
  typedef maidsafe::detail::BoundedString<NodeId::kSize * 2 + detail::PaddedWidth::value,
                                          NodeId::kSize * 2 + detail::PaddedWidth::value>
      FixedWidthString;

  explicit DataManagerKey(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};

void swap(DataManagerKey& lhs, DataManagerKey& rhs) MAIDSAFE_NOEXCEPT;
bool operator==(const DataManagerKey& lhs, const DataManagerKey& rhs);
bool operator!=(const DataManagerKey& lhs, const DataManagerKey& rhs);
bool operator<(const DataManagerKey& lhs, const DataManagerKey& rhs);
bool operator>(const DataManagerKey& lhs, const DataManagerKey& rhs);
bool operator<=(const DataManagerKey& lhs, const DataManagerKey& rhs);
bool operator>=(const DataManagerKey& lhs, const DataManagerKey& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_KEY_H_
