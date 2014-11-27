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

#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class DataManagerValueTest : public testing::Test {
 public:
  DataManagerValueTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")) {}

 protected:
  const maidsafe::test::TestPath kTestRoot_;
};

TEST_F(DataManagerValueTest, BEH_BlockingEmptySerialisation) {
  DataManager::Value value;
  EXPECT_ANY_THROW(value.Serialise());
}

TEST_F(DataManagerValueTest, BEH_ChunkSize) {
  {
    DataManager::Value value;
    EXPECT_EQ(0, value.chunk_size());
    uint64_t chunk_size(RandomUint32());
    value.SetChunkSize(chunk_size);
    EXPECT_EQ(chunk_size, value.chunk_size());
  }
  {
    uint64_t chunk_size(RandomUint32());
    DataManager::Value value(chunk_size);
    EXPECT_EQ(chunk_size, value.chunk_size());
  }
}

TEST_F(DataManagerValueTest, BEH_AddPmid) {
  PmidName pmid_node(Identity(RandomString(64)));
  // Add from constructor
  DataManager::Value value(kTestChunkSize);
  EXPECT_EQ(0, value.AllPmids().size());
  // Duplication Add
  value.AddPmid(pmid_node);
  EXPECT_EQ(1, value.AllPmids().size());
  EXPECT_EQ(pmid_node, value.AllPmids().front());
  // Add new one
  PmidName new_node(Identity(RandomString(64)));
  value.AddPmid(new_node);
  EXPECT_EQ(2, value.AllPmids().size());
  EXPECT_EQ(pmid_node, value.AllPmids().front());
  EXPECT_EQ(new_node, value.AllPmids().back());
}

TEST_F(DataManagerValueTest, BEH_RemovePmid) {
  PmidName removing_node(Identity(RandomString(64)));
  DataManager::Value value;
  // remove a non-existing entry from empty
  EXPECT_NO_THROW(value.RemovePmid(removing_node));
  EXPECT_TRUE(value.AllPmids().empty());
  // remove a non existing entry from a list
  PmidName pmid_node(Identity(RandomString(64)));
  value.AddPmid(pmid_node);
  EXPECT_NO_THROW(value.RemovePmid(removing_node));
  EXPECT_EQ(1, value.AllPmids().size());
  EXPECT_EQ(pmid_node, value.AllPmids().front());
  // remove an existing entry
  value.AddPmid(removing_node);
  EXPECT_EQ(2, value.AllPmids().size());
  EXPECT_NO_THROW(value.RemovePmid(removing_node));
  EXPECT_EQ(1, value.AllPmids().size());
  EXPECT_EQ(pmid_node, value.AllPmids().front());
  // remove the same entry again
  EXPECT_NO_THROW(value.RemovePmid(removing_node));
  EXPECT_EQ(1, value.AllPmids().size());
  EXPECT_EQ(pmid_node, value.AllPmids().front());
}

TEST_F(DataManagerValueTest, BEH_HasTarget) {
  PmidName target(Identity(RandomString(64)));
  PmidName pmid_node(Identity(RandomString(64)));

  DataManager::Value value(kTestChunkSize);
  EXPECT_FALSE(value.HasTarget(target));
  value.AddPmid(target);
  EXPECT_TRUE(value.HasTarget(target));
}

TEST_F(DataManagerValueTest, BEH_OnlinePmids) {
  DataManager::Value value;
  std::vector<PmidName> all_pmids, expected_online_pmids;
  { // get from empty regarding empty input
    std::vector<NodeId> online_pmids;
    EXPECT_TRUE(value.online_pmids(online_pmids).empty());
  }
  { // get from empty regarding full input
    for (size_t i(0); i < routing::Parameters::closest_nodes_size; ++i) {
      PmidName pmid_node(Identity(RandomString(64)));
      all_pmids.push_back(pmid_node);
      if ((RandomInt32() % 2) == 0)
        expected_online_pmids.push_back(pmid_node);
    }
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : all_pmids)
      online_pmids.push_back(NodeId(pmid_node->string()));
    EXPECT_TRUE(value.online_pmids(online_pmids).empty());
  }
  { // populated value regarding emput input
    for (auto& pmid_node : all_pmids)
      value.AddPmid(pmid_node);
    EXPECT_EQ(all_pmids.size(), value.AllPmids().size());
    std::vector<NodeId> online_pmids;
    EXPECT_TRUE(value.online_pmids(online_pmids).empty());
  }
  { // populated value regarding full input
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : expected_online_pmids)
      online_pmids.push_back(NodeId(pmid_node->string()));
    auto result(value.online_pmids(online_pmids));
    EXPECT_EQ(expected_online_pmids.size(), result.size());
    // order shall be retained
    for (size_t i(0); i < result.size(); ++i)
      EXPECT_EQ(expected_online_pmids[i], result[i]);
  }
  { // populated value regarding partial input
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : expected_online_pmids) {
      online_pmids.push_back(NodeId(RandomString(64)));
      online_pmids.push_back(NodeId(pmid_node->string()));
    }
    auto result(value.online_pmids(online_pmids));
    EXPECT_EQ(expected_online_pmids.size(), result.size());
    // order shall be retained
    for (size_t i(0); i < result.size(); ++i)
      EXPECT_EQ(expected_online_pmids[i], result[i]);
  }
}

TEST_F(DataManagerValueTest, BEH_NeedToPrune) {
  DataManager::Value value;
  std::vector<PmidName> all_pmids, expected_online_pmids, expected_offline_pmids;
  { // prune from empty regarding empty input
    std::vector<NodeId> online_pmids;
    PmidName pmid_node_to_prune;
    EXPECT_FALSE(value.NeedToPrune(online_pmids, pmid_node_to_prune));
  }
  { // get from empty regarding full input
    for (size_t i(0); i < routing::Parameters::closest_nodes_size; ++i) {
      PmidName pmid_node(Identity(RandomString(64)));
      all_pmids.push_back(pmid_node);
      if ((RandomInt32() % 2) == 0)
        expected_online_pmids.push_back(pmid_node);
      else
        expected_offline_pmids.push_back(pmid_node);
    }
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : all_pmids)
      online_pmids.push_back(NodeId(pmid_node->string()));
    PmidName pmid_node_to_prune;
    EXPECT_FALSE(value.NeedToPrune(online_pmids, pmid_node_to_prune));
  }
  { // full populated value regarding emput input
    for (auto& pmid_node : all_pmids)
      value.AddPmid(pmid_node);
    EXPECT_EQ(all_pmids.size(), value.AllPmids().size());
    std::vector<NodeId> online_pmids;
    PmidName pmid_node_to_prune;
    EXPECT_TRUE(value.NeedToPrune(online_pmids, pmid_node_to_prune));
    // value is full, even nothing happens, the oldest one shall be pruned
    EXPECT_EQ(all_pmids.front(), pmid_node_to_prune);
  }
  { // full populated value regarding full input
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : expected_online_pmids)
      online_pmids.push_back(NodeId(pmid_node->string()));
    PmidName pmid_node_to_prune;
    EXPECT_TRUE(value.NeedToPrune(online_pmids, pmid_node_to_prune));
    EXPECT_EQ(expected_offline_pmids.front(), pmid_node_to_prune);
  }
  { // full populated value regarding partial input
    value.AddPmid(all_pmids.front());
    std::vector<NodeId> online_pmids;
    for (auto& pmid_node : expected_online_pmids) {
      online_pmids.push_back(NodeId(RandomString(64)));
      online_pmids.push_back(NodeId(pmid_node->string()));
    }
    PmidName pmid_node_to_prune;
    EXPECT_TRUE(value.NeedToPrune(online_pmids, pmid_node_to_prune));
    EXPECT_EQ(expected_offline_pmids.front(), pmid_node_to_prune);
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe


