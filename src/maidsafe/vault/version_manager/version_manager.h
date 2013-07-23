

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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_

#include <cstdint>

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/version_manager/key.h"


namespace maidsafe {

namespace vault {

struct ActionPutVersion;
struct ActionGetVersion;
struct ActionGetBranch;
struct ActionDeleteBranchUntilFork;

}  // namespace vault


namespace nfs {

template<>
struct PersonaTypes<Persona::kVersionManager> {
  static const Persona persona = Persona::kVersionManager;
  typedef vault::VersionManagerKey Key;
  typedef StructuredDataVersions Value;
  typedef vault::UnresolvedAction<Key, vault::ActionPutVersion> UnresolvedPutVersion;
  typedef vault::UnresolvedAction<Key, vault::ActionGetVersion> UnresolvedGetVersion;
  typedef vault::UnresolvedAction<Key, vault::ActionGetBranch> UnresolvedGetBranch;
  typedef vault::UnresolvedAction<Key, vault::ActionDeleteBranchUntilFork>
      UnresolvedDeleteBranchUntilFork;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kVersionManager> VersionManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_
