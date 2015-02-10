/*  Copyright 2015 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_H_

#include <map>
#include <utility>
#include <vector>

#include "boost/expected/expected.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/key.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/mpid_manager/action_put_alert.h"
#include "maidsafe/vault/mpid_manager/action_delete_alert.h"
#include "maidsafe/vault/mpid_manager/action_put_message.h"
#include "maidsafe/vault/mpid_manager/action_delete_message.h"
#include "maidsafe/vault/action_create_remove_account.h"

namespace maidsafe {

namespace vault {

template <bool Remove>
struct ActionCreateRemoveAccount;
using ActionCreateAccount = ActionCreateRemoveAccount<false>;
using ActionRemoveAccount = ActionCreateRemoveAccount<true>;

}  // namespace vault

namespace nfs {

template <>
struct PersonaTypes<Persona::kMpidManager> {
  using GroupName = passport::PublicMpid::Name;
  using MessageKey = ImmutableData::Name;
  using GKPair = std::pair<GroupName, MessageKey>;
  using TransferInfo = std::map<NodeId, std::vector<GKPair>>;

  using Key = passport::PublicMpid::Name;
  using SyncKey = vault::GroupKey<Key>;
  using SyncGroupKey = vault::MetadataKey<Key>;
  using UnresolvedCreateAccount = vault::UnresolvedAction<SyncGroupKey, vault::ActionCreateAccount>;
  using UnresolvedRemoveAccount = vault::UnresolvedAction<SyncGroupKey, vault::ActionRemoveAccount>;
  using UnresolvedPutAlert = vault::UnresolvedAction<SyncGroupKey,
                                                     vault::ActionMpidManagerPutAlert>;
  using UnresolvedDeleteAlert = vault::UnresolvedAction<SyncGroupKey,
                                                        vault::ActionMpidManagerDeleteAlert>;
  using UnresolvedPutMessage = vault::UnresolvedAction<SyncGroupKey,
                                                        vault::ActionMpidManagerPutMessage>;
  using UnresolvedDeleteMessage = vault::UnresolvedAction<SyncGroupKey,
                                                        vault::ActionMpidManagerDeleteMessage>;
  static const Persona persona = Persona::kMpidManager;
};

}  // namespace nfs

namespace vault {

using DbMessageQueryResult = boost::expected<nfs_vault::MpidMessage, maidsafe_error>;

using  MpidManager = nfs::PersonaTypes<nfs::Persona::kMpidManager>;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_H_

