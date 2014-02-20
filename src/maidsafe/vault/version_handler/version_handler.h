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
#include "maidsafe/vault/version_handler/action_create_version_tree.h"
#include "maidsafe/vault/version_handler/action_put.h"
#include "maidsafe/vault/version_handler/action_delete_branch_until_fork.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

struct ActionVersionHandlerCreateVersionTree;
struct ActionVersionHandlerPut;
struct ActionVersionHandlerDeleteBranchUntilFork;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kVersionHandler> {
  static const Persona persona = Persona::kVersionHandler;
  typedef vault::VersionHandlerKey Key;
  typedef vault::VersionHandlerValue Value;
  typedef StructuredDataVersions::VersionName VersionName;
  typedef vault::UnresolvedAction<Key, vault::ActionVersionHandlerCreateVersionTree>
      UnresolvedCreateVersionTree;
  typedef vault::UnresolvedAction<Key, vault::ActionVersionHandlerPut> UnresolvedPutVersion;
  typedef vault::UnresolvedAction<Key, vault::ActionVersionHandlerDeleteBranchUntilFork>
      UnresolvedDeleteBranchUntilFork;
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kVersionHandler> VersionHandler;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_
