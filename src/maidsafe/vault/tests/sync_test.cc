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

TEST(SyncTest, BEH_SingleAction) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(routing::Parameters::node_group_size);

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
        EXPECT_TRUE(resolved->key == key);
        EXPECT_TRUE(resolved->action == unresolved_actions[0].action);
      }
      auto unresolved_list = persona_nodes[i].sync.GetUnresolvedActions();
      if ((j >= i) && (j < (routing::Parameters::node_group_size -1U)))
        EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;;
    }
    EXPECT_TRUE(resolved_count == 1);
    EXPECT_TRUE(persona_nodes[i].sync.GetUnresolvedActions().size() == 0U);
  }
}

TEST(SyncTest, BEH_SingleActionRepeatedMessages) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(
                                                           routing::Parameters::node_group_size);

  typename MaidManager::Key key(maid_name, Identity(NodeId(NodeId::kRandomId).string()),
                                DataTagValue::kMaidValue);
  std::vector<MaidManager::UnresolvedPut> unresolved_actions;
  for (const auto& persona_node : persona_nodes)
    unresolved_actions.push_back(persona_node.CreateUnresolvedAction(key));
  for (auto i(0U); i != routing::Parameters::node_group_size; ++i) {
    int resolved_count(0);
    for (auto j(0U); j != routing::Parameters::node_group_size; ++j) {
      auto resolved_action = persona_nodes[i].RecieveUnresolvedAction(unresolved_actions[j]);
      if (resolved_action) {
        ++resolved_count;
        EXPECT_TRUE(j >= routing::Parameters::node_group_size -2U) << "i = " << i << " , j = " << j;
        EXPECT_TRUE(resolved_action->key == key);
        EXPECT_TRUE(resolved_action->action == unresolved_actions[0].action);
      }
      auto unresolved_list = persona_nodes[i].sync.GetUnresolvedActions();
      if ((j >= i) && (j < (routing::Parameters::node_group_size -1U))) {
        EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;
        // Add repeated messages
        for (auto k(0U); k <= j; ++k) {
          std::cout << "Adding repeated : " << "i = " << i << " , j = " << j << ", k= " << k << std::endl;
          auto resolved = persona_nodes[i].RecieveUnresolvedAction(unresolved_actions[k]);
          EXPECT_TRUE(resolved == nullptr);
        }
      }
    }
    EXPECT_TRUE(resolved_count == 1);
    EXPECT_TRUE(persona_nodes[i].sync.GetUnresolvedActions().size() == 0U);
  }
}

TEST(SyncTest, BEH_TwoActionSameKey) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(routing::Parameters::node_group_size);

  typename MaidManager::Key key(maid_name, Identity(NodeId(NodeId::kRandomId).string()),
                                DataTagValue::kMaidValue);
  std::vector<MaidManager::UnresolvedPut> unresolved_actions_1, unresolved_actions_2;
  for (const auto& persona_node : persona_nodes) {
    unresolved_actions_1.push_back(persona_node.CreateUnresolvedAction(key));
    unresolved_actions_2.push_back(persona_node.CreateUnresolvedAction(key));
  }

  for (auto i(0U); i != routing::Parameters::node_group_size; ++i) {
    std::vector<std::unique_ptr<MaidManager::UnresolvedPut>> resolved_vector;
    for (auto j(0U); j != routing::Parameters::node_group_size; ++j) {
      auto resolved1 = persona_nodes[i].RecieveUnresolvedAction(unresolved_actions_1[j]);
      if (resolved1) {
        resolved_vector.push_back(std::move(resolved1));
        std::cout << "resolved 1 added j : " << j << std::endl;
       }
      auto resolved2 = persona_nodes[i].RecieveUnresolvedAction(unresolved_actions_2[j]);
      if (resolved2) {
        resolved_vector.push_back(std::move(resolved2));
        std::cout << "resolved 2 added j : " << j << std::endl;
      }
    }
    EXPECT_TRUE(resolved_vector.size() == 2) << " resolved_vector.size() : "<< resolved_vector.size();
  }
}

TEST(SyncTest, BEH_MultipleSequentialAction) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(routing::Parameters::node_group_size);

  for(auto count(0U); count != 100; ++count) {
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
          EXPECT_TRUE(resolved->key == key);
          EXPECT_TRUE(resolved->action == unresolved_actions[0].action);
        }
        auto unresolved_list = persona_nodes[i].sync.GetUnresolvedActions();
        if ((j >= i) && (j < (routing::Parameters::node_group_size -1U)))
          EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;;
      }
      EXPECT_TRUE(resolved_count == 1);
      EXPECT_TRUE(persona_nodes[i].sync.GetUnresolvedActions().size() == 0U);
    }
  }
}

TEST(SyncTest, BEH_MultipleRandomAction) {
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  std::vector<PersonaNode<MaidManager::UnresolvedPut>> persona_nodes(routing::Parameters::node_group_size);

  std::vector<MaidManager::Key> keys;
  std::vector<MaidManager::UnresolvedPut> unresolved_actions;
  for(auto count(0U); count != 100; ++count) { // FIXME add random DataTagValue types
    keys.push_back(MaidManager::Key(maid_name, Identity(NodeId(NodeId::kRandomId).string()),
                                    DataTagValue::kMaidValue));
    for (const auto& persona_node : persona_nodes) {
      unresolved_actions.push_back(persona_node.CreateUnresolvedAction(keys.back()));
    }
  }

  std::vector<std::unique_ptr<MaidManager::UnresolvedPut>> resolved_vector;
  for (const auto& unresolved_action : unresolved_actions) {
    for (auto& persona_node : persona_nodes) {
      auto resolved = persona_node.RecieveUnresolvedAction(unresolved_action);
      if (resolved)
          resolved_vector.push_back(std::move(resolved));
    }
  }
  EXPECT_TRUE(resolved_vector.size() == 400) << resolved_vector.size();
}

}  // test

}  // namespace vault

}  // namespace maidsafe
