/*  Copyright 2014 MaidSafe.net limited

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

#include "maidsafe/common/test.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/routing/close_nodes_change.h"

#include "maidsafe/vault/data_manager/database.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class DataManagerDatabaseTest : public testing::Test {
 public:
  DataManagerDatabaseTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")) {}

 protected:
  const maidsafe::test::TestPath kTestRoot_;
};

TEST_F(DataManagerDatabaseTest, BEH_GetFromEmpty) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  EXPECT_ANY_THROW(db.Get(key));
}

TEST_F(DataManagerDatabaseTest, BEH_AddPmid) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));

  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));

  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node);
    db.Commit(key, action_add_pmid);
  }
  auto results(db.Get(key).AllPmids());
  EXPECT_EQ(pmid_nodes.size(), results.size());
  // The order of insertion shall get retained
  for (size_t i(0); i < results.size(); ++i)
    EXPECT_EQ(pmid_nodes[i], results[i]);
}

TEST_F(DataManagerDatabaseTest, BEH_AddDuplicatedPmid) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  PmidName pmid_name(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));

  ActionDataManagerAddPmid action_add_pmid(pmid_name);
  db.Commit(key, action_add_pmid);
  db.Commit(key, action_add_pmid);

  auto results(db.Get(key).AllPmids());
  EXPECT_EQ(1, results.size());
  EXPECT_EQ(results.back(), pmid_name);
}

TEST_F(DataManagerDatabaseTest, BEH_RemovePmid) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));

  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node);
    db.Commit(key, action_add_pmid);
  }
  std::vector<PmidName> entries_to_remove {pmid_nodes[0], pmid_nodes[9], pmid_nodes[3]};
  for (auto& entry_to_remove : entries_to_remove) {
    ActionDataManagerRemovePmid action_remove_pmid(entry_to_remove);
    db.Commit(key, action_remove_pmid);
  }

  auto results(db.Get(key).AllPmids());
  EXPECT_EQ(pmid_nodes.size() - entries_to_remove.size(), results.size());
  // The order of insertion shall get retained
  for (auto& entry_to_remove : entries_to_remove)
    pmid_nodes.erase(std::find(pmid_nodes.begin(), pmid_nodes.end(), entry_to_remove));
  for (size_t i(0); i < results.size(); ++i)
    EXPECT_EQ(pmid_nodes[i], results[i]);
}

TEST_F(DataManagerDatabaseTest, BEH_Delete) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  // Delete from empty
  nfs::MessageId message_id(RandomInt32());
  ActionDataManagerDelete action_delete(message_id);
  EXPECT_ANY_THROW(db.Commit(key, action_delete));

  db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));
  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node);
    db.Commit(key, action_add_pmid);
  }
  // failing deletion
  {
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    nfs::MessageId message_id(RandomInt32());
    ActionDataManagerDelete action_delete(message_id);
    EXPECT_ANY_THROW(db.Commit(key, action_delete));
  }
  // succeeding deletion
  auto deleted_value(db.Commit(key, action_delete));
  auto results(deleted_value->AllPmids());
  EXPECT_EQ(pmid_nodes.size(), results.size());
  EXPECT_ANY_THROW(db.Get(key));
}

TEST_F(DataManagerDatabaseTest, BEH_GetRelatedAccounts) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  PmidName pmid_name(Identity(RandomString(64)));
  { // from empty
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_TRUE(result.empty());
  }

  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));
  // target non-exists
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node);
    db.Commit(key, action_add_pmid);
  }
  auto result(db.GetRelatedAccounts(pmid_name));
  EXPECT_TRUE(result.empty());
  // target being the last
  {
    ActionDataManagerAddPmid action_add_pmid(pmid_name);
    db.Commit(key, action_add_pmid);
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 1);
  }
  { // target being the first
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    std::vector<PmidName> local_pmid_nodes;
    std::copy(std::begin(pmid_nodes), std::end(pmid_nodes), std::back_inserter(local_pmid_nodes));
    local_pmid_nodes[0] = pmid_name;
    for (auto& pmid_node : local_pmid_nodes) {
      ActionDataManagerAddPmid action_add_pmid(pmid_node);
      db.Commit(key, action_add_pmid);
    }
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 2);
  }
  { // target in the middle
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    std::vector<PmidName> local_pmid_nodes;
    std::copy(std::begin(pmid_nodes), std::end(pmid_nodes), std::back_inserter(local_pmid_nodes));
    local_pmid_nodes[3] = pmid_name;
    for (auto& pmid_node : local_pmid_nodes) {
      ActionDataManagerAddPmid action_add_pmid(pmid_node);
      db.Commit(key, action_add_pmid);
    }
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 3);
  }
  { // target being the only pmid_node
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    ActionDataManagerAddPmid action_add_pmid(pmid_name);
    db.Commit(key, action_add_pmid);
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 4);
  }
}

TEST_F(DataManagerDatabaseTest, BEH_GetTransferInfo) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  std::map<DataManager::Key, DataManager::Value> key_value_map;
  std::vector<NodeId> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(NodeId(RandomString(64)));
  // GetTransferInfo from CloseNodeChanges will force a rule that only the vault id is already
  // one of the closest then the transfer will happen, so here we will need to ensure :
  // 1, the db only contains entries that the vault is already in range
  // 2, when a new node joining, the vault is still in range for the entry
  //    (otherwise the corresponding entry will be removed)
  // In the test, we assume the vault having an id of pmid_nodes.front()
  auto vault_id(pmid_nodes.front());
  std::vector<NodeId> data_entries;
  for (auto& pmid_node : pmid_nodes) {
    std::vector<NodeId> holders(routing::Parameters::group_size);
    std::partial_sort_copy(std::begin(pmid_nodes), std::end(pmid_nodes),
                           std::begin(holders), std::end(holders),
                           [pmid_node](const NodeId& lhs, const NodeId& rhs) {
      return NodeId::CloserToTarget(lhs, rhs, pmid_node);
    });
    auto in_range(std::find(holders.begin(), holders.end(), vault_id));
    if (in_range != holders.end())
      data_entries.push_back(pmid_node);
  }
  LOG(kVerbose) << " created data entries : " << data_entries.size();
  for (auto& account : data_entries) {
    PmidName pmid_node(Identity(account.string()));
    DataManager::Key key(Identity(pmid_node->string()), ImmutableData::Tag::kValue);
    DataManager::Value value(kTestChunkSize);
    value.AddPmid(PmidName(Identity(RandomString(64))));
    key_value_map[key] = value;
    auto pmids(value.AllPmids());
    db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
    for (auto& pmid : pmids) {
      ActionDataManagerAddPmid action_add_pmid(pmid);
      db.Commit(key, action_add_pmid);
    }
  }

  std::vector<NodeId> old_close_nodes_pass_down(pmid_nodes), new_close_nodes(pmid_nodes);
  old_close_nodes_pass_down.erase(std::find(old_close_nodes_pass_down.begin(),
                                            old_close_nodes_pass_down.end(),
                                            vault_id));
  NodeId new_node(RandomString(64));
  new_close_nodes.push_back(new_node);
  std::vector<NodeId> new_close_nodes_pass_down(new_close_nodes);
  new_close_nodes_pass_down.erase(std::find(new_close_nodes_pass_down.begin(),
                                            new_close_nodes_pass_down.end(),
                                            vault_id));

  std::shared_ptr<routing::CloseNodesChange> close_node_change_ptr(new
      routing::CloseNodesChange(vault_id, old_close_nodes_pass_down, new_close_nodes_pass_down));
  LOG(kVerbose) << " db.GetTransferInfo " ;
  DataManager::TransferInfo result(db.GetTransferInfo(close_node_change_ptr));
  size_t expected_entries(0);
  for (auto& pmid_node : data_entries) {
    std::vector<NodeId> new_holders(routing::Parameters::group_size);
    std::partial_sort_copy(std::begin(new_close_nodes), std::end(new_close_nodes),
                           std::begin(new_holders), std::end(new_holders),
                           [pmid_node](const NodeId& lhs, const NodeId& rhs) {
      return NodeId::CloserToTarget(lhs, rhs, pmid_node);
    });
    auto in_range(std::find(new_holders.begin(), new_holders.end(), new_node));
    if (in_range != new_holders.end())
      ++expected_entries;
  }
  LOG(kVerbose) << " expected_entries : " << expected_entries;
  if (expected_entries > 0) {
    EXPECT_EQ(1, result.size());
    EXPECT_EQ(new_node, result.begin()->first);
    EXPECT_EQ(expected_entries, result.begin()->second.size());
  } else {
    EXPECT_TRUE(result.empty());
  }

  // those entry that vault is no longe closest shall got pruned
  std::vector<DataManager::Key> pruned;
  for (auto& pmid_node : data_entries) {
    std::vector<NodeId> holders(routing::Parameters::group_size);
    std::partial_sort_copy(std::begin(new_close_nodes), std::end(new_close_nodes),
                           std::begin(holders), std::end(holders),
                           [pmid_node](const NodeId& lhs, const NodeId& rhs) {
      return NodeId::CloserToTarget(lhs, rhs, pmid_node);
    });
    auto in_range(std::find(holders.begin(), holders.end(), vault_id));
    if (in_range == holders.end()) {
      DataManager::Key key(Identity(pmid_node.string()), ImmutableData::Tag::kValue);
      pruned.push_back(key);
    }
  }
  LOG(kVerbose) << " expected to be pruned : " << pruned.size();
  for (auto& key : pruned)
    EXPECT_ANY_THROW(db.Get(key));
}

TEST_F(DataManagerDatabaseTest, BEH_HandleTransfer) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  DataManager::Value value;
  value.SetChunkSize(kTestChunkSize);
  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i) {
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));
    value.AddPmid(pmid_nodes.back());
  }
  PmidName pmid_name(pmid_nodes[3]);
  { // Handle Transfer
    std::vector<DataManager::KvPair> transferred;
    transferred.push_back(std::make_pair<>(key, value));
    db.HandleTransfer(transferred);
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 1);
    auto result_pmids(db.Get(key).AllPmids());
    for (size_t i(0); i < result_pmids.size(); ++i)
      EXPECT_EQ(pmid_nodes[i], result_pmids[i]);
  }
  { // Handle Duplicated Transfer
    DataManager::Value duplicated_value(value);
    std::vector<DataManager::KvPair> transferred;
    transferred.push_back(std::make_pair<>(key, duplicated_value));
    db.HandleTransfer(transferred);
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 1);
    auto result_pmids(db.Get(key).AllPmids());
    for (size_t i(0); i < result_pmids.size(); ++i)
      EXPECT_EQ(pmid_nodes[i], result_pmids[i]);
  }
  { // Handle one more Transfer
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    std::vector<DataManager::KvPair> transferred;
    transferred.push_back(std::make_pair<>(key, value));
    db.HandleTransfer(transferred);
    auto result(db.GetRelatedAccounts(pmid_name));
    EXPECT_EQ(result.size(), 2);
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe

