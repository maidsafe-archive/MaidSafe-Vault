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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_PUT_VERSION_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_PUT_VERSION_H_


#include <tuple>

#include "maidsafe/structured_data_versions.h"

namespace maidsafe {

namespace vault {
template<typename StoragePolicy>
struct PutVersion : public StoragePolicy {
  PutVersion(const StructuredDataVersions& old_version_in,
             const StructuredDataVersions& new_version_in,
             const Key& key_in) 
      : old_version(old_version_in),
        new_version(new_version_in) {}
  explicit PutVersion(const serialised_type& serialised_put_data)
      : old_version(old_version_in),
        new_version(new_version_in) {}

  bool operator()() {
    auto value = StoragePolicy::Get(key);
    value.PutVersion(old_version, new_version);
    StoragePolicy::Put(make_pair(key, value));
  }

  bool operator==(const PutVersion& lhs, const PutVersion& rhs) {
    return std::tie(lhs.old_version, lhs.new_version, lhs.key) == 
           std::tie(rhs.old_version, rhs.new_version, rhs.key);
  }
  bool operator!=(const PutVersion& lhs, const PutVersion& rhs) {
    return !operator==(lhs, rhs);
  }
  Serialise();
  static const int action_id = []{ return 1; }
  StructuredDataVersions old_version;
  StructuredDataVersions new_version;
  Key key;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_PUT_VERSION_H_

