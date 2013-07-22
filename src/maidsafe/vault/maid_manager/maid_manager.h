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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_

#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/maid_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionMaidManagerPut;
struct ActionMaidManagerDelete;
struct ActionRegisterPmid;
struct ActionUnregisterPmid;

}  // namespace vault

namespace nfs {

template<>
struct PersonaTypes<Persona::kMaidManager> {
  static const Persona persona = Persona::kMaidManager;
  typedef vault::GroupKey<passport::PublicMaid::name_type> Key;
  typedef vault::MaidManagerValue Value;
  typedef vault::UnresolvedAction<Key, vault::ActionMaidManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionMaidManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionRegisterPmid> UnresolvedRegisterPmid;
  typedef vault::UnresolvedAction<Key, vault::ActionUnregisterPmid> UnresolvedUnregisterPmid;

  enum class Action : int32_t {
    kPut,
    kDelete,
    kRegisterPmid,
    kUnregisterPmid,
    kSync,
    kAccountTransfer
  };
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kMaidManager> MaidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
