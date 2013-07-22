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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_

#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/pmid_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionPmidManagerPut;
struct ActionPmidManagerDelete;
struct ActionGetPmidTotals;

}  // namespace vault


namespace nfs {

template<>
struct PersonaTypes<Persona::kPmidManager> {
  static const Persona persona = Persona::kPmidManager;
  typedef vault::GroupKey<passport::PublicPmid::name_type> Key;
  typedef vault::PmidManagerValue Value;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionGetPmidTotals> UnresolvedGetPmidTotals;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kPmidManager> PmidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_
