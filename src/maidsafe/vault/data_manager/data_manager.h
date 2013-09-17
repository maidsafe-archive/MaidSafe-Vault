/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/action_add_pmid.h"
#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_node_down.h"
#include "maidsafe/vault/data_manager/action_node_up.h"
#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/action_remove_pmid.h"

namespace maidsafe {

namespace vault {

class Metadata;

//struct ActionDataManagerPut;
//struct ActionDataManagerDelete;
//struct ActionDataManagerAddPmid;
//struct ActionDataManagerRemovePmid;
//struct ActionDataManagerNodeUp;
//struct ActionDataManagerNodeDown;

}  // namespace vault


namespace nfs {

template<>
struct PersonaTypes<Persona::kDataManager> {
  static const Persona persona = Persona::kDataManager;
  typedef vault::Key Key;
  typedef vault::DataManagerValue Value;
  typedef vault::Metadata Metadata;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerAddPmid> UnresolvedAddPmid;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerRemovePmid> UnresolvedRemovePmid;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerNodeUp> UnresolvedNodeUp;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerNodeDown> UnresolvedNodeDown;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kDataManager> DataManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
