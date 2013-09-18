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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_UNRESOLVED_ENTRY_VALUE_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_UNRESOLVED_ENTRY_VALUE_H_

#include "boost/optional.hpp"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/data_types/structured_data_versions.h"


namespace maidsafe {

namespace vault {

struct VersionManagerUnresolvedEntryValue {
  VersionManagerUnresolvedEntryValue();
  VersionManagerUnresolvedEntryValue(const VersionManagerUnresolvedEntryValue& other);
  VersionManagerUnresolvedEntryValue& operator=(VersionManagerUnresolvedEntryValue other);
  // TODO(Team) when we get std::optional we can have move ctr but boost prevents this just now
  boost::optional<StructuredDataVersions::VersionName> version;
  boost::optional<StructuredDataVersions::VersionName> new_version;
  boost::optional<routing::ReplyFunctor> reply_functor;
  boost::optional<StructuredDataVersions::serialised_type> serialised_db_value;  // account xfer
};

// no move in boost::optional uncomment when std::optional is available
// void swap(const VersionManagerUnresolvedEntryValue& lhs,
//           const VersionManagerUnresolvedEntryValue& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs);

bool operator!=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs);

bool operator<(const VersionManagerUnresolvedEntryValue& lhs,
               const VersionManagerUnresolvedEntryValue& rhs);

bool operator>(const VersionManagerUnresolvedEntryValue& lhs,
               const VersionManagerUnresolvedEntryValue& rhs);

bool operator<=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs);

bool operator>=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_UNRESOLVED_ENTRY_VALUE_H_
