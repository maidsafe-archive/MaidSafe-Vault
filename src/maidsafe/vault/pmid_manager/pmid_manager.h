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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_

#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/pmid_manager/action_put.h"
#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

struct ActionPmidManagerPut;
struct ActionPmidManagerDelete;
struct ActionGetPmidTotals;

struct PmidManagerMetadata;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kPmidManager> {
  static const Persona persona = Persona::kPmidManager;
  typedef vault::GroupKey<passport::PublicPmid::Name> Key;
  typedef passport::PublicPmid::Name GroupName;
  typedef vault::PmidManagerValue Value;
  typedef vault::PmidManagerMetadata Metadata;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionGetPmidTotals> UnresolvedGetPmidTotals;

  enum class Action : int32_t {
    kPut,
    kGet,
    kDelete,
    kGetPmidTotals,
    kSync,
    kAccountTransfer
  };
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kPmidManager> PmidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_
