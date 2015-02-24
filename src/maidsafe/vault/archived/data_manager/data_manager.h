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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/unresolved_account_transfer_action.h"
#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_add_pmid.h"
#include "maidsafe/vault/data_manager/action_remove_pmid.h"

namespace maidsafe {

namespace vault {

struct ActionDataManagerAddPmid;
struct ActionDataManagerPut;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kDataManager> {
  static const Persona persona = Persona::kDataManager;
  using Key = vault::Key;
  using Value = vault::DataManagerValue;
  using KvPair = std::pair<Key, Value>;
  using TransferInfo = std::map<NodeId, std::vector<KvPair>>;
  using UnresolvedPut = vault::UnresolvedAction<Key, vault::ActionDataManagerPut>;
  using UnresolvedDelete = vault::UnresolvedAction<Key, vault::ActionDataManagerDelete>;
  using UnresolvedAddPmid = vault::UnresolvedAction<Key, vault::ActionDataManagerAddPmid>;
  using UnresolvedRemovePmid = vault::UnresolvedAction<Key, vault::ActionDataManagerRemovePmid>;
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kDataManager> DataManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
