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



#include <atomic>
#include <algorithm>

#include "boost/progress.hpp"

#include "maidsafe/vault/group_db.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/passport.h"

#include "maidsafe/nfs/vault/pmid_registration.h"
#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/key.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/version_handler/value.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

// template <typename UnresolvedActionType>
// struct PersonaNode {
//  PersonaNode() : node_id(NodeId::IdType::kRandomId), sync(node_id), resolved_count(0) {}
//
//  UnresolvedActionType CreateUnresolvedAction(
//      const typename UnresolvedActionType::KeyType& key) const {
//    typename UnresolvedActionType::ActionType action(100);
//    return UnresolvedActionType(key, action, node_id);
//  }
//
//  std::unique_ptr<UnresolvedActionType> ReceiveUnresolvedAction(
//      const UnresolvedActionType& unresolved_action) {
//    auto received_unresolved_action = UnresolvedActionType(
//        unresolved_action.Serialise(), unresolved_action.this_node_and_entry_id->first, node_id);
//    auto resolved(sync.AddUnresolvedAction(received_unresolved_action));
//    if (resolved)
//      ++resolved_count;
//    return std::move(resolved);
//  }
//
//  NodeId node_id;
//  Sync<UnresolvedActionType> sync;
//  std::atomic<int> resolved_count;
//
// private:
//  PersonaNode(PersonaNode&&);
//  PersonaNode(const PersonaNode&);
//  PersonaNode& operator=(PersonaNode other);
// };
//
//
// std::vector<MaidManager::Key> CreateKeys(int count, int group_count = 20) {
//  std::vector<passport::PublicMaid::Name> group_vector;
//  for (auto i(0); i < group_count; ++i) {
//    auto maid(passport::CreateMaidAndSigner().first);
//    passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//    group_vector.push_back(maid_name);
//  }
//  std::vector<MaidManager::Key> keys;
//  for (auto i(0); i < count; ++i) {
//    MaidManager::Key key(group_vector[i % group_count],
//                         Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                         DataTagValue::kMaidValue);
//    keys.push_back(key);
//  }
//  return keys;
// }
//
//
// TEST(SyncTest, BEH_Constructor) {
//  Sync<MaidManager::UnresolvedPut> maid_manager_sync_puts((NodeId(NodeId::IdType::kRandomId)));
//  Sync<MaidManager::UnresolvedCreateAccount> maid_manager_sync_create_account(
//      (NodeId(NodeId::IdType::kRandomId)));
//  Sync<PmidManager::UnresolvedPut> pmid_manager_sync_puts((NodeId(NodeId::IdType::kRandomId)));
//  Sync<DataManager::UnresolvedPut> data_manager_sync_puts((NodeId(NodeId::IdType::kRandomId)));
// }
//
// TEST(SyncTest, BEH_SingleAction) {
//  typedef std::unique_ptr<PersonaNode<MaidManager::UnresolvedPut>> PersonaNodePtr;
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  std::vector<PersonaNodePtr> persona_nodes(routing::Parameters::group_size);
//  std::generate(std::begin(persona_nodes), std::end(persona_nodes),
//                [] { return PersonaNodePtr(new PersonaNodePtr::element_type); });
//
//  MaidManager::Key key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                       DataTagValue::kMaidValue);
//  std::vector<MaidManager::UnresolvedPut> unresolved_actions;
//  for (const auto& persona_node : persona_nodes)
//    unresolved_actions.push_back(persona_node->CreateUnresolvedAction(key));
//
//  for (auto i(0U); i != routing::Parameters::group_size; ++i) {
//    int resolved_count(0);
//    for (auto j(0U); j != routing::Parameters::group_size; ++j) {
//      auto resolved = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions[j]);
//      if (resolved) {
//        ++resolved_count;
//        EXPECT_TRUE(j >= routing::Parameters::group_size - 2U) << "i = " << i << " , j = " << j;
//        EXPECT_TRUE(resolved->key == key);
//        EXPECT_TRUE(resolved->action == unresolved_actions[0].action);
//      }
//      auto unresolved_list = persona_nodes[i]->sync.GetUnresolvedActions();
//      if ((j >= i) && (j < (routing::Parameters::group_size - 1U)))
//        EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;
//    }
//    EXPECT_TRUE(resolved_count == 1);
//    EXPECT_TRUE(persona_nodes[i]->sync.GetUnresolvedActions().empty());
//  }
// }
//
// TEST(SyncTest, BEH_SingleActionRepeatedMessages) {
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  typedef std::unique_ptr<PersonaNode<MaidManager::UnresolvedPut>> PersonaNodePtr;
//  std::vector<PersonaNodePtr> persona_nodes(routing::Parameters::group_size);
//  std::generate(std::begin(persona_nodes), std::end(persona_nodes),
//                [] { return PersonaNodePtr(new PersonaNodePtr::element_type); });
//
//  MaidManager::Key key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                       DataTagValue::kMaidValue);
//  std::vector<MaidManager::UnresolvedPut> unresolved_actions;
//  for (const auto& persona_node : persona_nodes)
//    unresolved_actions.push_back(persona_node->CreateUnresolvedAction(key));
//  for (auto i(0U); i != routing::Parameters::group_size; ++i) {
//    int resolved_count(0);
//    for (auto j(0U); j != routing::Parameters::group_size; ++j) {
//      auto resolved_action = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions[j]);
//      if (resolved_action) {
//        ++resolved_count;
//        EXPECT_TRUE(j >= routing::Parameters::group_size - 2U) << "i = " << i << " , j = " << j;
//        EXPECT_TRUE(resolved_action->key == key);
//        EXPECT_TRUE(resolved_action->action == unresolved_actions[0].action);
//      }
//      auto unresolved_list = persona_nodes[i]->sync.GetUnresolvedActions();
//      if ((j >= i) && (j < (routing::Parameters::group_size - 1U))) {
//        EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;
//        // Add repeated messages
//        for (auto k(0U); k <= j; ++k) {
//          auto resolved = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions[k]);
//          EXPECT_TRUE(resolved == nullptr);
//        }
//      }
//    }
//    EXPECT_TRUE(resolved_count == 1);
//    EXPECT_TRUE(persona_nodes[i]->sync.GetUnresolvedActions().empty());
//  }
// }
//
// TEST(SyncTest, BEH_TwoActionSameKey) {
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  typedef std::unique_ptr<PersonaNode<MaidManager::UnresolvedPut>> PersonaNodePtr;
//  std::vector<PersonaNodePtr> persona_nodes(routing::Parameters::group_size);
//  std::generate(std::begin(persona_nodes), std::end(persona_nodes),
//                [] { return PersonaNodePtr(new PersonaNodePtr::element_type); });
//
//  MaidManager::Key key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                       DataTagValue::kMaidValue);
//  std::vector<MaidManager::UnresolvedPut> unresolved_actions_1, unresolved_actions_2;
//  for (const auto& persona_node : persona_nodes) {
//    unresolved_actions_1.push_back(persona_node->CreateUnresolvedAction(key));
//    unresolved_actions_2.push_back(persona_node->CreateUnresolvedAction(key));
//  }
//
//  for (auto i(0U); i != routing::Parameters::group_size; ++i) {
//    std::vector<std::unique_ptr<MaidManager::UnresolvedPut>> resolved_vector;
//    for (auto j(0U); j != routing::Parameters::group_size; ++j) {
//      auto resolved1 = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions_1[j]);
//      if (resolved1) {
//        resolved_vector.push_back(std::move(resolved1));
//      }
//      auto resolved2 = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions_2[j]);
//      if (resolved2) {
//        resolved_vector.push_back(std::move(resolved2));
//      }
//    }
//    EXPECT_TRUE(resolved_vector.size() == 2)
//        << "resolved_vector.size(): " << resolved_vector.size();
//  }
// }
//
// TEST(SyncTest, BEH_MultipleSequentialAction) {
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  typedef std::unique_ptr<PersonaNode<MaidManager::UnresolvedPut>> PersonaNodePtr;
//  std::vector<PersonaNodePtr> persona_nodes(routing::Parameters::group_size);
//  std::generate(std::begin(persona_nodes), std::end(persona_nodes),
//                [] { return PersonaNodePtr(new PersonaNodePtr::element_type); });
//
//  for (auto count(0U); count != 100; ++count) {
//    MaidManager::Key key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                         DataTagValue::kMaidValue);
//    std::vector<MaidManager::UnresolvedPut> unresolved_actions;
//    for (const auto& persona_node : persona_nodes)
//      unresolved_actions.push_back(persona_node->CreateUnresolvedAction(key));
//    for (auto i(0U); i != routing::Parameters::group_size; ++i) {
//      int resolved_count(0);
//      for (auto j(0U); j != routing::Parameters::group_size; ++j) {
//        auto resolved = persona_nodes[i]->ReceiveUnresolvedAction(unresolved_actions[j]);
//        if (resolved) {
//          ++resolved_count;
//          EXPECT_TRUE(j >= routing::Parameters::group_size - 2U) << "i = " << i << " , j = " << j;
//          EXPECT_TRUE(resolved->key == key);
//          EXPECT_TRUE(resolved->action == unresolved_actions[0].action);
//        }
//        auto unresolved_list = persona_nodes[i]->sync.GetUnresolvedActions();
//        if ((j >= i) && (j < (routing::Parameters::group_size - 1U)))
//          EXPECT_TRUE(unresolved_list.size() == 1U) << "i = " << i << " , j = " << j;
//      }
//      EXPECT_TRUE(resolved_count == 1);
//      EXPECT_TRUE(persona_nodes[i]->sync.GetUnresolvedActions().empty());
//    }
//  }
// }
//
// TEST(SyncTest, BEH_MultipleRandomAction) {
//  const int kActionCount(500);
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  typedef std::unique_ptr<PersonaNode<MaidManager::UnresolvedPut>> PersonaNodePtr;
//  std::vector<PersonaNodePtr> persona_nodes(routing::Parameters::group_size);
//  std::generate(std::begin(persona_nodes), std::end(persona_nodes),
//                [] { return PersonaNodePtr(new PersonaNodePtr::element_type); });
//
//  std::vector<MaidManager::Key> keys;
//  std::vector<std::unique_ptr<MaidManager::UnresolvedPut>> unresolved_actions;
//  for (auto count(0); count != kActionCount; ++count) {  // FIXME add random DataTagValue types
//    keys.push_back(MaidManager::Key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                                    DataTagValue::kMaidValue));
//    for (const auto& persona_node : persona_nodes) {
//      std::unique_ptr<MaidManager::UnresolvedPut> action(
//          new MaidManager::UnresolvedPut(persona_node->CreateUnresolvedAction(keys.back())));
//      unresolved_actions.push_back(std::move(action));
//    }
//  }
//
//  std::random_shuffle(unresolved_actions.begin(), unresolved_actions.end());
//
//  std::vector<std::unique_ptr<MaidManager::UnresolvedPut>> resolved_vector;
//  for (const auto& unresolved_action : unresolved_actions) {
//    for (auto& persona_node : persona_nodes) {
//      auto resolved = persona_node->ReceiveUnresolvedAction(*unresolved_action);
//      if (resolved)
//        resolved_vector.push_back(std::move(resolved));
//    }
//  }
//  EXPECT_EQ(resolved_vector.size(),
//            static_cast<size_t>(kActionCount * routing::Parameters::group_size))
//      << resolved_vector.size();
//  int count(0);
//  for (auto& key : keys) {
//    unsigned int matches(0);
//    for (const auto& resolved : resolved_vector) {
//      if (resolved->key == key)
//        ++matches;
//    }
//    EXPECT_TRUE(matches == routing::Parameters::group_size) << matches;
//    ++count;
//  }
//  EXPECT_TRUE(count == kActionCount) << count;
//  for (auto& persona_node : persona_nodes) {
//    EXPECT_TRUE(persona_node->resolved_count == kActionCount);
//  }
// }
//
//
//// different group
//// repeated keys
//// mixed keys
// template <typename Persona, typename KeyType>
// void ApplySyncToPersona(Persona& persona_node, std::vector<KeyType> keys) {
//  for (auto& key : keys) {
//    auto unresolved_action = persona_node.CreateUnresolvedAction(key);
//    persona_node.ReceiveUnresolvedAction(unresolved_action);
//  }
// }
//
// TEST(SyncTest, FUNC_MultipleParallelRandomAction) {
//  const int kActionCount(1000);
//  auto keys = CreateKeys(kActionCount);
//  //  auto itr = std::begin(keys);
//  //  std::vector<MaidManager::Key> keys_thread_1(itr, itr + (kActionCount / 4));
//  //  std::advance(itr, (kActionCount / 4));
//  //  std::vector<MaidManager::Key> keys_thread_2(itr, itr + (kActionCount / 4));
//  //  std::advance(itr, (kActionCount / 4));
//  //  std::vector<MaidManager::Key> keys_thread_3(itr, itr + (kActionCount / 4));
//  //  std::advance(itr, (kActionCount / 4));
//  //  std::vector<MaidManager::Key> keys_thread_4(itr, itr + (kActionCount / 4));
//  PersonaNode<MaidManager::UnresolvedPut> persona_node;
//  ApplySyncToPersona(persona_node, keys);
// }

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
