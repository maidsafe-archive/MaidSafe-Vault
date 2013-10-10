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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_KEY_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_KEY_H_

#include <string>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/key_utils.h"

namespace maidsafe {

namespace vault {

struct VersionHandlerKey {
  template <typename Data>
  VersionHandlerKey(const typename Data::Name& name_in, const Identity& originator_in)
      : name(name_in.data), type(Data::type_enum_value()), originator(originator_in) {}
  explicit VersionHandlerKey(const std::string& serialised_key);
  VersionHandlerKey(const VersionHandlerKey& other);
  VersionHandlerKey(VersionHandlerKey&& other);
  VersionHandlerKey& operator=(VersionHandlerKey other);
  std::string Serialise() const;

  Identity name;
  DataTagValue type;
  Identity originator;

 private:
  typedef maidsafe::detail::BoundedString<NodeId::kSize * 2 + detail::PaddedWidth::value,
                                          NodeId::kSize * 2 + detail::PaddedWidth::value>
      FixedWidthString;

  explicit VersionHandlerKey(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};

void swap(VersionHandlerKey& lhs, VersionHandlerKey& rhs) MAIDSAFE_NOEXCEPT;
bool operator==(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);
bool operator!=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);
bool operator<(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);
bool operator>(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);
bool operator<=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);
bool operator>=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_KEY_H_
