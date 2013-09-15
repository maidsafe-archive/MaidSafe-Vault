/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_


#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"


namespace maidsafe {
namespace vault {

//template <typename Data>
//void MaidManagerService::Sync(const Identity& data_name,
//                              const NodeId& sender,
//                              const nfs::MessageAction& action) {
//  nfs::PersonaTypes<Persona::kMaidManager>::Key group_key(
//      nfs::PersonaTypes<Persona::kMaidManager>::GroupName(sender),
//      data_name,
//      Data::Name::data_type);
//  nfs::UnresolvedPut unresolved_action(group_key, action);
//  sync_puts_.AddLocalAction(unresolved_action);
//  DoSync();
//}




}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_
