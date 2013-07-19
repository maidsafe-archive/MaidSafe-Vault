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

#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/data_types/structured_data_versions.h"

#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

struct ActionPutVersion {
  ActionPutVersion(const StructuredDataVersions::VersionName& old_version_in,
                   const StructuredDataVersions::VersionName& new_version_in)
      : old_version(old_version_in),
        new_version(new_version_in) {}
  explicit ActionPutVersion(const std::string& serialised_action);
  ActionPutVersion(const ActionPutVersion& other);
  ActionPutVersion(ActionPutVersion&& other);

  template<typename Storage>
  maidsafe_error operator()(Storage& storage, const VersionManager::Key& key) const;

  std::string Serialise() const;

  static const VersionManager::Action action_id;
  const StructuredDataVersions::VersionName old_version, new_version;

 private:
  ActionPutVersion();
  ActionPutVersion& operator=(ActionPutVersion other);
};

bool operator==(const ActionPutVersion& lhs, const ActionPutVersion& rhs);

bool operator!=(const ActionPutVersion& lhs, const ActionPutVersion& rhs);



template<typename Storage>
maidsafe_error ActionPutVersion::operator()(Storage& storage,
                                            const VersionManager::Key& key) const {
  try {
    auto value = storage.Get(key);
    value.PutVersion(old_version, new_version);
    storage.Put(key, value);
  }
  catch(const maidsafe_error& error) {
    LOG(kError) << "Caught exception executing ActionPutVersion: " << error.what();
    return error;
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception executing ActionPutVersion: " << error.what();
    return MakeError(CommonErrors::unknown);
  }
  return MakeError(CommonErrors::success);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_PUT_VERSION_H_

