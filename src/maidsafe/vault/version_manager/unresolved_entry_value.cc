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

#include "maidsafe/vault/version_manager/unresolved_entry_value.h"

#include <tuple>

namespace maidsafe {

namespace vault {

VersionManagerUnresolvedEntryValue::VersionManagerUnresolvedEntryValue()
    : version(),
      new_version(),
      reply_functor(),
      serialised_db_value() {}

VersionManagerUnresolvedEntryValue::VersionManagerUnresolvedEntryValue(
    const VersionManagerUnresolvedEntryValue& other)
        : version(other.version),
          new_version(other.new_version),
          reply_functor(other.reply_functor),
          serialised_db_value(other.serialised_db_value) {}

VersionManagerUnresolvedEntryValue& VersionManagerUnresolvedEntryValue::operator=(
    VersionManagerUnresolvedEntryValue other) {
  version = other.version;
  new_version = other.new_version;
  reply_functor = other.reply_functor;
  serialised_db_value = other.serialised_db_value;
  // TODO(dirvine) need to check that we should not parse and
  // compare serialised VersionManagerVersions
  return *this;
}

// no move in boost::optional uncomment when std::optional is available
// void swap(const VersionManagerUnresolvedEntryValue& lhs,
//           const VersionManagerUnresolvedEntryValue& rhs) MAIDSAFE_NOEXCEPT {
//  using std::swap;
//  swap(lhs.version, rhs.version);
//  swap(lhs.new_version, rhs.new_version);
//  swap(lhs.reply_functor, rhs.reply_functor);
//  swap(lhs.serialised_db_value, rhs.serialised_db_value);
// }

bool operator==(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs) {
  return lhs.version == rhs.version &&
         lhs.new_version == rhs.new_version &&
         lhs.serialised_db_value == rhs.serialised_db_value;
}

bool operator!=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const VersionManagerUnresolvedEntryValue& lhs,
               const VersionManagerUnresolvedEntryValue& rhs) {
  return std::tie(lhs.version, lhs.new_version, lhs.serialised_db_value) <
         std::tie(rhs.version, rhs.new_version, rhs.serialised_db_value);
}

bool operator>(const VersionManagerUnresolvedEntryValue& lhs,
               const VersionManagerUnresolvedEntryValue& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const VersionManagerUnresolvedEntryValue& lhs,
                const VersionManagerUnresolvedEntryValue& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
