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

#include "maidsafe/vault/data_manager/datamanager_database.h"
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

  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node, kTestChunkSize);
    db.Commit(key, action_add_pmid);
  }
  auto results(db.Get(key).AllPmids());
  EXPECT_EQ(pmid_nodes.size(), results.size());
  // The order of insertion shall get retained
  for (size_t i(0); i < results.size(); ++i)
    EXPECT_EQ(pmid_nodes[i], results[i]);
}

TEST_F(DataManagerDatabaseTest, BEH_RemovePmid) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i)
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));

  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node, kTestChunkSize);
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

  std::vector<PmidName> pmid_nodes;
  for (int i(0); i < 10; ++i) {
    pmid_nodes.push_back(PmidName(Identity(RandomString(64))));
    LOG(kVerbose) << "Generated : " << HexSubstr(pmid_nodes.back()->string());
  }
  for (auto& pmid_node : pmid_nodes) {
    ActionDataManagerAddPmid action_add_pmid(pmid_node, kTestChunkSize);
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


TEST_F(DataManagerDatabaseTest, BEH_GetRelatedAccountsFromEmpty) {
  DataManagerDataBase db(UniqueDbPath(*kTestRoot_));
  PmidName pmid_name(Identity(RandomString(64)));
  auto result(db.GetRelatedAccounts(pmid_name));
  EXPECT_TRUE(result.empty());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe

