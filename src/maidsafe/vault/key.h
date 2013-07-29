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

#ifndef MAIDSAFE_VAULT_KEY_H_
#define MAIDSAFE_VAULT_KEY_H_

#include <string>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

namespace test {
class KeyTest_BEH_Serialise_Test;
class KeyTest_BEH_All_Test;
}  // namespace test

template<typename Persona>
class ManagerDb;

struct Key {
  Key(const Identity& name_in, DataTagValue type_in);
  explicit Key(const std::string& serialised_key);
  Key(const Key& other);
  Key(Key&& other);
  Key& operator=(Key other);
  std::string Serialise() const;

  Identity name;
  DataTagValue type;

  template<typename Persona>
  friend class ManagerDb;

 private:
  typedef maidsafe::detail::BoundedString<
      NodeId::kSize + detail::PaddedWidth::value,
      NodeId::kSize + detail::PaddedWidth::value> FixedWidthString;

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
