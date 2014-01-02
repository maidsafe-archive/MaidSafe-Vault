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

#include "maidsafe/vault/group_db.h"

#include "boost/progress.hpp"

#include "leveldb/db.h"
#include "leveldb/options.h"

#include "leveldb/status.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/vault/pmid_registration.h"
#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/version_handler/key.h"
#include "maidsafe/vault/version_handler/value.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

template <typename UnresolvedActionType>
struct PersonaNode {
  PersonaNode() : sync(), node_id(NodeId::kRandomId) {}

  UnresolvedActionType CreateUnresolvedAction(const typename UnresolvedActionType::KeyType& key) const {
    typename UnresolvedActionType::ActionType action(100);
    return UnresolvedActionType(key, action, node_id);
  }

  std::unique_ptr<UnresolvedActionType> RecieveUnresolvedAction(
      const UnresolvedActionType& unresolved_action) {
    auto recieved_unresolved_action = UnresolvedActionType(unresolved_action.Serialise(),
        unresolved_action.this_node_and_entry_id->first, node_id);
    return std::move(sync.AddUnresolvedAction(recieved_unresolved_action));
  }

  Sync<UnresolvedActionType> sync;
  NodeId node_id;
};

TEST(SyncTest, BEH_Constructor) {
  Sync<MaidManager::UnresolvedPut> maid_manager_sync_puts;
  Sync<MaidManager::UnresolvedCreateAccount> maid_manager_sync_create_account;
  Sync<PmidManager::UnresolvedPut> pmid_manager_sync_puts;
  Sync<PmidManager::UnresolvedSetPmidHealth> pmid_manager_sync_set_pmid_health;
  Sync<DataManager::UnresolvedPut> data_manager_sync_puts;
}

TEST(SyncTest, BEH_SimpleOperation) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(4);

  typename MaidManager::Key key(maid_name, Identity(NodeId(NodeId::kRandomId).string()),
                                DataTagValue::kMaidValue);
  std::vector<MaidManager::UnresolvedPut> unresolved_actions;
  for (const auto& persona_node : persona_nodes)
    unresolved_actions.push_back(persona_node.CreateUnresolvedAction(key));
  for (auto i(0U); i != routing::Parameters::node_group_size; ++i) {
    int resolved_count(0);
    for (auto j(0U); j != routing::Parameters::node_group_size; ++j) {
      auto resolved = persona_nodes[i].RecieveUnresolvedAction(unresolved_actions[j]);
      if (resolved) {
        ++resolved_count;
        EXPECT_TRUE(j >= routing::Parameters::node_group_size -2U) << "i = " << i << " , j = " << j;
      }
      auto unresolved_list = persona_nodes[0].sync.GetUnresolvedActions();
//      if (resolved_count == 1)  // FIXME
//        EXPECT_TRUE(unresolved_list.size() == 0U);


    }
    EXPECT_TRUE(resolved_count == 1);
    EXPECT_TRUE(persona_nodes[i].sync.GetUnresolvedActions().size() == 0U);
  }
}

//    EXPECT_TRUE(persona_nodes[i].RecieveUnresolvedAction(unresolved_actions[j]) == nullptr);
//    EXPECT_TRUE(persona_nodes[0].RecieveUnresolvedAction(unresolved_actions[1]) == nullptr);
//    EXPECT_TRUE(persona_nodes[0].RecieveUnresolvedAction(unresolved_actions[2]) != nullptr);
//    EXPECT_TRUE(persona_nodes[0].RecieveUnresolvedAction(unresolved_actions[3]) == nullptr);
//    EXPECT_TRUE(persona_nodes[0].sync.GetUnresolvedActions().size() == 0U);


}  // test

}  // namespace vault

}  // namespace maidsafe
