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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_

#include "boost/optional.hpp"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/data_types/structured_data_versions.h"


namespace maidsafe {

namespace vault {

struct StructuredDataUnresolvedEntryValue {
  StructuredDataUnresolvedEntryValue();
  StructuredDataUnresolvedEntryValue(const StructuredDataUnresolvedEntryValue& other);
  StructuredDataUnresolvedEntryValue& operator=(StructuredDataUnresolvedEntryValue other);
  // TODO(Team) when we get std::optional we can have move ctr but boost prevents this just now
  boost::optional<StructuredDataVersions::VersionName> version;
  boost::optional<StructuredDataVersions::VersionName> new_version;
  boost::optional<routing::ReplyFunctor> reply_functor;
  boost::optional<StructuredDataVersions::serialised_type> serialised_db_value;  // account xfer
};

// no move in boost::optional uncomment when std::optional is available
// void swap(const StructuredDataUnresolvedEntryValue& lhs,
//           const StructuredDataUnresolvedEntryValue& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator!=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator<(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs);

bool operator>(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs);

bool operator<=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator>=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_
