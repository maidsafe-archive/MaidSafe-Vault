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
