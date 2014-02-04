/*  Copyright 2009 MaidSafe.net limited

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

#include "maidsafe/vault/action_account_transfer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/routing/parameters.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/data_manager/data_manager.h"



namespace maidsafe {

namespace vault {

namespace test {

TEST(AccountTransferTest, BEH_Constructor) {
  typedef vault::UnresolvedAction<DataManager::Key, ActionAccountTransfer<std::string>>
       UnresolvedAccountTransfer;
  Sync<UnresolvedAccountTransfer> sync_account_transfer((NodeId(NodeId::kRandomId)));
  ActionAccountTransfer<std::string> action("value");
  DataManager::Key key(Identity(NodeId(NodeId::kRandomId).string()), DataTagValue::kMaidValue);

  UnresolvedAccountTransfer unresolved_action(key, action, NodeId(NodeId::kRandomId));
  sync_account_transfer.AddUnresolvedAction(unresolved_action);
  sync_account_transfer.AddUnresolvedAction(unresolved_action);

}



}  // test

}  // namespace vault

}  // namespace maidsafe
