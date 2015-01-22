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

#include "maidsafe/common/test.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/routing/close_nodes_change.h"

#include "maidsafe/vault/mpid_manager/mpid_manager_database.h"
#include "maidsafe/vault/mpid_manager/mpid_manager.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class MpidManagerDatabaseTest : public testing::Test {
 public:
  MpidManagerDatabaseTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")) {}

 protected:
  const maidsafe::test::TestPath kTestRoot_;
};

TEST_F(MpidManagerDatabaseTest, BEH_Put) {
  MpidManagerDataBase db(UniqueDbPath(*kTestRoot_));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  MpidName mpid(Identity(RandomString(64)));
  EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
  EXPECT_TRUE(db.Has(data.name()));
  // Double Put
  EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
  auto entries(db.GetEntriesForMPID(mpid));
  EXPECT_EQ(1, entries.size());
  EXPECT_EQ(data.name(), entries.front());
  auto statistics(db.GetStatistic(mpid));
  EXPECT_EQ(1, statistics.first);
  EXPECT_EQ(kTestChunkSize, statistics.second);
}

TEST_F(MpidManagerDatabaseTest, BEH_Delete) {
  MpidManagerDataBase db(UniqueDbPath(*kTestRoot_));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  MpidManager::MessageKey key(data.name());
  // Delete Empty
  EXPECT_NO_THROW(db.Delete(key));
  // Delete Existing Entry
  MpidName mpid(Identity(RandomString(64)));
  EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
  EXPECT_TRUE(db.Has(data.name()));
  EXPECT_NO_THROW(db.Delete(key));
  EXPECT_FALSE(db.Has(data.name()));
  // Put After Delete
  EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
  EXPECT_TRUE(db.Has(data.name()));
  auto entries(db.GetEntriesForMPID(mpid));
  EXPECT_EQ(1, entries.size());
}

TEST_F(MpidManagerDatabaseTest, BEH_HasGroup) {
  MpidManagerDataBase db(UniqueDbPath(*kTestRoot_));
  MpidName mpid(Identity(RandomString(64)));
  // HasGroup check for non-existing mpid account
  EXPECT_FALSE(db.HasGroup(mpid));
  // After first put
  ImmutableData data_1(NonEmptyString(RandomString(kTestChunkSize)));
  EXPECT_NO_THROW(db.Put(data_1.name(), data_1.data().string().size(), mpid));
  EXPECT_TRUE(db.HasGroup(mpid));
  // After second put
  ImmutableData data_2(NonEmptyString(RandomString(kTestChunkSize)));
  EXPECT_NO_THROW(db.Put(data_2.name(), data_2.data().string().size(), mpid));
  EXPECT_TRUE(db.HasGroup(mpid));
  // After first delete
  EXPECT_NO_THROW(db.Delete(data_1.name()));
  EXPECT_TRUE(db.HasGroup(mpid));
  // After second delete
  EXPECT_NO_THROW(db.Delete(data_2.name()));
  EXPECT_FALSE(db.HasGroup(mpid));
  // after another Put
  ImmutableData data_3(NonEmptyString(RandomString(kTestChunkSize)));
  EXPECT_NO_THROW(db.Put(data_3.name(), data_3.data().string().size(), mpid));
  EXPECT_TRUE(db.HasGroup(mpid));
}

TEST_F(MpidManagerDatabaseTest, BEH_GetEntriesForMPID) {
  MpidManagerDataBase db(UniqueDbPath(*kTestRoot_));
  for (size_t i(0); i < 10; ++i) {
    MpidName mpid(Identity(RandomString(64)));
    size_t num_of_entries(RandomUint32() % 5);
    for (size_t j(0); j < num_of_entries; ++j) {
      ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
      EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
      EXPECT_TRUE(db.Has(data.name()));
    }
    auto entries(db.GetEntriesForMPID(mpid));
    EXPECT_EQ(num_of_entries, entries.size());
  }
}

TEST_F(MpidManagerDatabaseTest, BEH_GetStatistic) {
  MpidManagerDataBase db(UniqueDbPath(*kTestRoot_));
  for (size_t i(0); i < 10; ++i) {
    MpidName mpid(Identity(RandomString(64)));
    size_t num_of_entries(RandomUint32() % 5);
    size_t total_size(0);
    for (size_t j(0); j < num_of_entries; ++j) {
      size_t data_size(RandomUint32() % kTestChunkSize);
      total_size += data_size;
      ImmutableData data(NonEmptyString(RandomString(data_size)));
      EXPECT_NO_THROW(db.Put(data.name(), data.data().string().size(), mpid));
      EXPECT_TRUE(db.Has(data.name()));
    }
    auto statistics(db.GetStatistic(mpid));
    EXPECT_EQ(num_of_entries, statistics.first);
    EXPECT_EQ(total_size, statistics.second);
  }
}

//TEST_F(DataManagerDatabaseTest, BEH_GetTransferInfo) {
//  DataManagerDatabase db(UniqueDbPath(*kTestRoot_));
//  std::map<DataManager::Key, DataManager::Value> key_value_map;
//  std::vector<NodeId> pmid_nodes;
//  for (int i(0); i < 10; ++i) {
//    pmid_nodes.push_back(NodeId(RandomString(64)));
//    LOG(kVerbose) << "Generated pmid node " << HexSubstr(pmid_nodes.back().string());
//  }
//  // GetTransferInfo from CloseNodeChanges will force a rule that only the vault id is already
//  // one of the closest then the transfer will happen, so here we will need to ensure :
//  // 1, the db only contains entries that the vault is already in range
//  // 2, when a new node joining, the vault is still in range for the entry
//  //    (otherwise the corresponding entry will be removed)
//  // In the test, we assume the vault having an id of pmid_nodes.front()
//  auto vault_id(pmid_nodes.front());
//  std::vector<NodeId> data_entries;
//  for (auto& pmid_node : pmid_nodes) {
//    // DM shall not hold account bearing the same name to itself
//    if (vault_id != pmid_node) {
//      std::vector<NodeId> candidates(pmid_nodes);
//      candidates.erase(std::find(candidates.begin(), candidates.end(), pmid_node));
//      std::vector<NodeId> holders(routing::Parameters::group_size);
//      std::partial_sort_copy(std::begin(candidates), std::end(candidates),
//                             std::begin(holders), std::end(holders),
//                             [pmid_node](const NodeId& lhs, const NodeId& rhs) {
//        return NodeId::CloserToTarget(lhs, rhs, pmid_node);
//      });
//      auto in_range(std::find(holders.begin(), holders.end(), vault_id));
//      if (in_range != holders.end()) {
//        data_entries.push_back(pmid_node);
//        LOG(kVerbose) << "created data entry : " << HexSubstr(data_entries.back().string());
//      }
//    }
//  }
//  LOG(kVerbose) << " created data entries : " << data_entries.size();
//  for (auto& account : data_entries) {
//    PmidName pmid_node(Identity(account.string()));
//    DataManager::Key key(Identity(pmid_node->string()), ImmutableData::Tag::kValue);
//    DataManager::Value value(kTestChunkSize);
//    value.AddPmid(PmidName(Identity(RandomString(64))));
//    key_value_map[key] = value;
//    auto pmids(value.AllPmids());
//    db.Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
//    for (auto& pmid : pmids) {
//      ActionDataManagerAddPmid action_add_pmid(pmid);
//      db.Commit(key, action_add_pmid);
//    }
//  }

//  std::vector<NodeId> old_close_nodes_pass_down(pmid_nodes), new_close_nodes(pmid_nodes);
//  old_close_nodes_pass_down.erase(std::find(old_close_nodes_pass_down.begin(),
//                                            old_close_nodes_pass_down.end(),
//                                            vault_id));
//  NodeId new_node(RandomString(64));
//  LOG(kVerbose) << "new_node : " << HexSubstr(new_node.string());
//  new_close_nodes.push_back(new_node);
//  std::vector<NodeId> new_close_nodes_pass_down(new_close_nodes);
//  new_close_nodes_pass_down.erase(std::find(new_close_nodes_pass_down.begin(),
//                                            new_close_nodes_pass_down.end(),
//                                            vault_id));

//  std::shared_ptr<routing::CloseNodesChange> close_node_change_ptr(new
//      routing::CloseNodesChange(vault_id, old_close_nodes_pass_down, new_close_nodes_pass_down));
//  LOG(kVerbose) << " db.GetTransferInfo ";
//  DataManager::TransferInfo result(db.GetTransferInfo(close_node_change_ptr));

//  size_t expected_entries(0);
//  std::vector<DataManager::Key> pruned;
//  for (auto& pmid_node : data_entries) {
//    // DM shall not hold account bearing the same name to itself
//    // but shall take vault_id into calculation
//    std::vector<NodeId> candidates(new_close_nodes);
//    candidates.erase(std::find(candidates.begin(), candidates.end(), pmid_node));
//    std::vector<NodeId> new_holders(routing::Parameters::group_size);
//    std::partial_sort_copy(std::begin(candidates), std::end(candidates),
//                           std::begin(new_holders), std::end(new_holders),
//                           [pmid_node](const NodeId& lhs, const NodeId& rhs) {
//      return NodeId::CloserToTarget(lhs, rhs, pmid_node);
//    });
//    // those entry that the current vault is no longer among the closest shall got pruned
//    // and the vault shall not transfer the entry to the new node even it is the new holder
//    auto in_range(std::find(new_holders.begin(), new_holders.end(), vault_id));
//    if (in_range == new_holders.end()) {
//      LOG(kVerbose) << "expected removed account : " << HexSubstr(pmid_node.string());
//      DataManager::Key key(Identity(pmid_node.string()), ImmutableData::Tag::kValue);
//      pruned.push_back(key);
//    } else {
//      auto in_range(std::find(new_holders.begin(), new_holders.end(), new_node));
//      if (in_range != new_holders.end()) {
//        LOG(kVerbose) << "expected having entry of account " << HexSubstr(pmid_node.string());
//        ++expected_entries;
//      }
//    }
//  }
//  LOG(kVerbose) << " expected_entries : " << expected_entries;
//  if (expected_entries > 0) {
//    EXPECT_EQ(1, result.size());
//    EXPECT_EQ(new_node, result.begin()->first);
//    EXPECT_EQ(expected_entries, result.begin()->second.size());
//  } else {
//    EXPECT_TRUE(result.empty());
//  }
//  LOG(kVerbose) << " expected to be pruned : " << pruned.size();
//  for (auto& key : pruned)
//    EXPECT_ANY_THROW(db.Get(key));
//}

//TEST_F(DataManagerDatabaseTest, BEH_HandleTransfer) {
//  DataManagerDatabase db(UniqueDbPath(*kTestRoot_));
//  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
//  DataManager::Key key(data.name());
//  DataManager::Value value;
//  value.SetChunkSize(kTestChunkSize);
//  std::vector<PmidName> pmid_nodes;
//  for (int i(0); i < 10; ++i) {
//    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));
//    value.AddPmid(pmid_nodes.back());
//  }
//  PmidName pmid_name(pmid_nodes[3]);
//  { // Handle Transfer
//    std::vector<DataManager::KvPair> transferred;
//    transferred.push_back(std::make_pair(key, value));
//    db.HandleTransfer(transferred);
//    auto result(db.GetRelatedAccounts(pmid_name));
//    EXPECT_EQ(result.size(), 1);
//    auto result_pmids(db.Get(key).AllPmids());
//    for (size_t i(0); i < result_pmids.size(); ++i)
//      EXPECT_EQ(pmid_nodes[i], result_pmids[i]);
//  }
//  { // Handle Duplicated Transfer
//    DataManager::Value duplicated_value(value);
//    std::vector<DataManager::KvPair> transferred;
//    transferred.push_back(std::make_pair(key, duplicated_value));
//    db.HandleTransfer(transferred);
//    auto result(db.GetRelatedAccounts(pmid_name));
//    EXPECT_EQ(result.size(), 1);
//    auto result_pmids(db.Get(key).AllPmids());
//    for (size_t i(0); i < result_pmids.size(); ++i)
//      EXPECT_EQ(pmid_nodes[i], result_pmids[i]);
//  }
//  { // Handle one more Transfer
//    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
//    DataManager::Key key(data.name());
//    std::vector<DataManager::KvPair> transferred;
//    transferred.push_back(std::make_pair(key, value));
//    db.HandleTransfer(transferred);
//    auto result(db.GetRelatedAccounts(pmid_name));
//    EXPECT_EQ(result.size(), 2);
//  }
//}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe

