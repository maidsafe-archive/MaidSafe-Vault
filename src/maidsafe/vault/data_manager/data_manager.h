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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionDataManagerPut;
struct ActionDataManagerGet;
struct ActionDataManagerDelete;
struct ActionNodeDown;
struct ActionNodeUp;

}  // namespace vault


namespace nfs {

template<>
struct PersonaTypes<Persona::kDataManager> {
  static const Persona persona = Persona::kDataManager;
  typedef vault::Key Key;
  typedef vault::DataManagerValue Value;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerGet> UnresolvedGet;
  typedef vault::UnresolvedAction<Key, vault::ActionDataManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionNodeDown> UnresolvedNodeDown;
  typedef vault::UnresolvedAction<Key, vault::ActionNodeUp> UnresolvedNodeUp;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kDataManager> DataManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_H_
