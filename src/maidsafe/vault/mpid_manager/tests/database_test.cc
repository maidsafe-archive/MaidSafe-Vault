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

#include "maidsafe/vault/mpid_manager/database.h"
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
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
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
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
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
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
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
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
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
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
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

TEST_F(MpidManagerDatabaseTest, BEH_GetTransferInfo) {
  MpidManagerDatabase db(UniqueDbPath(*kTestRoot_));
  std::map<MpidName, std::vector<ImmutableData::Name>> mpid_dataname_map;
  std::vector<NodeId> mpid_nodes;
  for (size_t i(0); i < 10; ++i) {
    MpidName mpid(Identity(RandomString(64)));
    std::vector<ImmutableData::Name> data_names;
    size_t num_of_entries(RandomUint32() % 5 + 1);
    for (size_t j(0); j < num_of_entries; ++j)
      data_names.push_back(ImmutableData::Name(Identity(RandomString(64))));
    mpid_dataname_map.insert(std::make_pair(mpid, std::move(data_names)));
    mpid_nodes.push_back(NodeId(mpid->string()));
  }

  // GetTransferInfo from CloseNodeChanges will force a rule that only the vault id is already
  // one of the closest then the transfer will happen, so here we will need to ensure :
  // 1, the db only contains entries that the vault is already in range
  // 2, when a new node joining, the vault is still in range for the entry
  //    (otherwise the corresponding entry will be removed)
  NodeId vault_id(RandomString(64));
  std::vector<NodeId> account_entries;
  for (auto& mpid_node : mpid_nodes) {
    std::vector<NodeId> candidates(mpid_nodes);
    candidates.push_back(vault_id);
    candidates.erase(std::find(candidates.begin(), candidates.end(), mpid_node));
    std::vector<NodeId> holders(routing::Parameters::group_size);
    std::partial_sort_copy(std::begin(candidates), std::end(candidates),
                           std::begin(holders), std::end(holders),
                           [mpid_node](const NodeId& lhs, const NodeId& rhs) {
      return NodeId::CloserToTarget(lhs, rhs, mpid_node);
    });
    auto in_range(std::find(holders.begin(), holders.end(), vault_id));
    if (in_range != holders.end())
      account_entries.push_back(mpid_node);
  }
  for (auto& account : account_entries) {
    MpidName mpid(Identity(account.string()));
    for (auto& data_name : mpid_dataname_map[mpid]) {
      EXPECT_NO_THROW(db.Put(data_name, RandomUint32() % kTestChunkSize, mpid));
      EXPECT_TRUE(db.Has(data_name));
    }
  }

  std::vector<NodeId> old_close_nodes_pass_down(mpid_nodes), new_close_nodes(mpid_nodes);
  NodeId new_node(RandomString(64));
  new_close_nodes.push_back(new_node);
  std::vector<NodeId> new_close_nodes_pass_down(new_close_nodes);
  new_close_nodes.push_back(vault_id);

  std::shared_ptr<routing::CloseNodesChange> close_node_change_ptr(new
      routing::CloseNodesChange(vault_id, old_close_nodes_pass_down, new_close_nodes_pass_down));
  MpidManager::DbTransferInfo result(db.GetTransferInfo(close_node_change_ptr));

  std::vector<MpidName> pruned, transferred;
  for (auto& mpid_node : account_entries) {
    // MpidManager shall not hold account bearing the same name to itself
    // but shall take vault_id into calculation
    std::vector<NodeId> candidates(new_close_nodes);
    candidates.erase(std::find(candidates.begin(), candidates.end(), mpid_node));
    std::vector<NodeId> new_holders(routing::Parameters::group_size);
    std::partial_sort_copy(std::begin(candidates), std::end(candidates),
                           std::begin(new_holders), std::end(new_holders),
                           [mpid_node](const NodeId& lhs, const NodeId& rhs) {
      return NodeId::CloserToTarget(lhs, rhs, mpid_node);
    });
    // those entry that the current vault is no longer among the closest shall got pruned
    // and the vault shall not transfer the entry to the new node even it is the new holder
    auto in_range(std::find(new_holders.begin(), new_holders.end(), vault_id));
    if (in_range == new_holders.end()) {
      pruned.push_back(MpidName(Identity(mpid_node.string())));
    } else {
      auto in_range(std::find(new_holders.begin(), new_holders.end(), new_node));
      if (in_range != new_holders.end())
        transferred.push_back(MpidName(Identity(mpid_node.string())));
    }
  }

  if (pruned.size() > 0) {
    size_t total_transferred_entries(0);
    for (auto& entry : pruned)
      total_transferred_entries += mpid_dataname_map[entry].size();
    EXPECT_EQ(total_transferred_entries, result[NodeId()].size());
  }
  if (transferred.size() > 0) {
    size_t total_transferred_entries(0);
    for (auto& entry : transferred)
      total_transferred_entries += mpid_dataname_map[entry].size();
    EXPECT_EQ(total_transferred_entries, result[new_node].size());
  }
  for (auto& mpid : pruned)
    EXPECT_FALSE(db.HasGroup(mpid));
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe

