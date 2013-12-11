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
#include "maidsafe/vault/metadata_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/pmid_manager/action_put.h"
#include "maidsafe/vault/pmid_manager/action_set_pmid_health.h"
#include "maidsafe/vault/pmid_manager/action_create_account.h"
#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

struct ActionPmidManagerPut;
struct ActionPmidManagerDelete;
struct ActionPmidManagerCreateAccount;
struct ActionGetPmidTotals;
struct ActionPmidManagerSetPmidHealth;
struct PmidManagerMetadata;
struct ActionCreatePmidAccount;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kPmidManager> {
  static const Persona persona = Persona::kPmidManager;
  typedef passport::PublicPmid::Name GroupName;
  typedef vault::GroupKey<GroupName> Key;
  typedef vault::PmidManagerValue Value;
  typedef vault::PmidManagerMetadata Metadata;
  typedef vault::MetadataKey<GroupName> MetadataKey;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerPut> UnresolvedPut;
  typedef vault::UnresolvedAction<Key, vault::ActionPmidManagerDelete> UnresolvedDelete;
  typedef vault::UnresolvedAction<Key, vault::ActionGetPmidTotals> UnresolvedGetPmidTotals;
  typedef vault::UnresolvedAction<
              MetadataKey, vault::ActionPmidManagerSetPmidHealth> UnresolvedSetPmidHealth;
  typedef vault::UnresolvedAction<
              MetadataKey, vault::ActionCreatePmidAccount> UnresolvedCreateAccount;
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kPmidManager> PmidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_H_
