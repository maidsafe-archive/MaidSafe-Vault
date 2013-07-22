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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_

#include <string>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

struct VersionManagerKey {
  template<typename Data>
  VersionManagerKey(const typename Data::name_type& name_in, const Identity& originator_in)
      : name(name_in.data),
        type(Data::type_enum_value()),
        originator(originator_in) {}
  explicit VersionManagerKey(const std::string& serialised_key);
  VersionManagerKey(const VersionManagerKey& other);
  VersionManagerKey(VersionManagerKey&& other);
  VersionManagerKey& operator=(VersionManagerKey other);
  std::string Serialise() const;

  Identity name;
  DataTagValue type;
  Identity originator;

 private:
  typedef maidsafe::detail::BoundedString<
      NodeId::kSize + detail::PaddedWidth::value,
      NodeId::kSize + detail::PaddedWidth::value> FixedWidthString;

  explicit VersionManagerKey(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};

void swap(VersionManagerKey& lhs, VersionManagerKey& rhs) MAIDSAFE_NOEXCEPT;
bool operator==(const VersionManagerKey& lhs, const VersionManagerKey& rhs);
bool operator!=(const VersionManagerKey& lhs, const VersionManagerKey& rhs);
bool operator<(const VersionManagerKey& lhs, const VersionManagerKey& rhs);
bool operator>(const VersionManagerKey& lhs, const VersionManagerKey& rhs);
bool operator<=(const VersionManagerKey& lhs, const VersionManagerKey& rhs);
bool operator>=(const VersionManagerKey& lhs, const VersionManagerKey& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
