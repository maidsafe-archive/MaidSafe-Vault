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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/key.h"
#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/metadata_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/unresolved_account_transfer_action.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/value.h"


namespace maidsafe {

namespace vault {

template <bool Remove>
struct ActionCreateRemoveAccount;
typedef ActionCreateRemoveAccount<false> ActionCreateAccount;
typedef ActionCreateRemoveAccount<true> ActionRemoveAccount;
struct ActionMaidManagerPut;
struct ActionMaidManagerDelete;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kMaidManager> {
  static const Persona persona = Persona::kMaidManager;
  using GroupName = passport::PublicMaid::Name;
  using Key = GroupName;
  using Value = vault::MaidManagerValue;
  using SyncKey = vault::GroupKey<GroupName>;
  using MetadataKey = vault::MetadataKey<GroupName>;
  using AccountType = std::pair<Key, Value>;
  using TransferInfo = std::map<NodeId, std::vector<AccountType>>;
  using UnresolvedCreateAccount = vault::UnresolvedAction<MetadataKey, vault::ActionCreateAccount>;
  using UnresolvedRemoveAccount = vault::UnresolvedAction<MetadataKey, vault::ActionRemoveAccount>;
  using UnresolvedPut = vault::UnresolvedAction<SyncKey, vault::ActionMaidManagerPut>;
  using UnresolvedDelete = vault::UnresolvedAction<SyncKey, vault::ActionMaidManagerDelete>;
  using UnresolvedAccountTransfer = vault::UnresolvedAccountTransferAction<GroupName, std::string>;
};

}  // namespace nfs

namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kMaidManager> MaidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
