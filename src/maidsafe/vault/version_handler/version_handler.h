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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_

#include <cstdint>

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/version_handler/key.h"

namespace maidsafe {

namespace vault {

struct ActionPutVersion;
struct ActionGetVersion;
struct ActionGetBranch;
struct ActionDeleteBranchUntilFork;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kVersionHandler> {
  static const Persona persona = Persona::kVersionHandler;
  typedef vault::VersionHandlerKey Key;
  typedef StructuredDataVersions Value;
  typedef vault::UnresolvedAction<Key, vault::ActionPutVersion> UnresolvedPutVersion;
  typedef vault::UnresolvedAction<Key, vault::ActionGetVersion> UnresolvedGetVersion;
  typedef vault::UnresolvedAction<Key, vault::ActionGetBranch> UnresolvedGetBranch;
  typedef vault::UnresolvedAction<Key, vault::ActionDeleteBranchUntilFork>
      UnresolvedDeleteBranchUntilFork;
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kVersionHandler> VersionHandler;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_
