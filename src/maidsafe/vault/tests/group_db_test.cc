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

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/value.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/version_handler/value.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

//PmidManagerValue CreatePmidManagerValue(const PmidName& pmid_name) {
//  PmidManagerValue metadata(pmid_name);
//  metadata.SetAvailableSize(0);
//  return metadata;
//}

//MaidManagerMetadata CreateMaidManagerMetadata(const passport::Maid& maid) {
//  std::vector<PmidTotals> pmid_totals_vector;
//  for (auto i(0); i != 1; ++i) {
//    auto pmid(passport::CreatePmidAndSigner().first);
//    auto pmid_metadata(CreatePmidManagerValue(PmidName(pmid.name())));
//    nfs_vault::PmidRegistration pmid_registration(maid, pmid, false);
//    PmidTotals pmid_totals(pmid_registration.Serialise(), pmid_metadata);
//    pmid_totals_vector.push_back(pmid_totals);
//  }
//  return MaidManagerMetadata(0, pmid_totals_vector);
//}

//// update metadata only

//// change value
//struct TestGroupDbActionModifyValue {
//  detail::DbAction operator()(MaidManagerMetadata& metadata,
//                              std::unique_ptr<MaidManagerValue>& value) {
//    if (!value)
//      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
//    value->Put(100);
//    metadata.PutData(100);
//    return detail::DbAction::kPut;
//  }
//};

//// put value
//struct TestGroupDbActionPutValue {
//  detail::DbAction operator()(MaidManagerMetadata& metadata,
//                              std::unique_ptr<MaidManagerValue>& value) {
//    if (!value)
//      value.reset(new MaidManagerValue());
//    value->Put(100);
//    metadata.PutData(100);
//    return detail::DbAction::kPut;
//  }
//};

//// delete value
//struct TestGroupDbActionDeleteValue {
//  detail::DbAction operator()(MaidManagerMetadata& metadata,
//                              std::unique_ptr<MaidManagerValue>& value) {
//    if (!value)
//      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));

//    metadata.DeleteData(value->Delete());
//    assert(value->count() >= 0);
//    if (value->count() == 0)
//      return detail::DbAction::kDelete;
//    else
//      return detail::DbAction::kPut;
//  }
//};

//// pmid manager

//// update metadata only

//// put value
//struct TestPmidGroupDbActionPutValue {
//  detail::DbAction operator()(PmidManagerValue& metadata,
//                              std::unique_ptr<PmidManagerValue>& value) {
//    if (!value) {
//      value.reset(new PmidManagerValue(100));
//    } else {
//      LOG(kError) << "data already exists";
//      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::data_already_exists));
//    }

//    metadata.PutData(value->size());
//    return detail::DbAction::kPut;
//  }
//};

//// delete value
//struct TestPmidGroupDbActionDeleteValue {
//  detail::DbAction operator()(PmidManagerValue& metadata,
//                              std::unique_ptr<PmidManagerValue>& value) {
//    if (!value)
//      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
//    metadata.DeleteData(value->size());
//    //    if (!pmid_node_available)
//    //      metadata.SetAvailableSize(0);
//    return detail::DbAction::kDelete;
//  }
//};


//// FIXME remove this utility once RunInParallel in common is fixed
//void RunDbTestInParallel(int thread_count, std::function<void()> functor) {
//  std::vector<std::thread> threads;
//  for (int i = 0; i < thread_count; ++i)
//    threads.push_back(std::move(std::thread([functor]() { functor(); })));
//  for (auto& thread : threads)
//    thread.join();
//}

//void RunMaidManagerGroupDbTest(GroupDb<MaidManager>& maid_group_db) {
//  auto maid(passport::CreateMaidAndSigner().first);
//  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
//  EXPECT_THROW(maid_group_db.GetMetadata(maid_name), maidsafe_error);
//  maid_group_db.DeleteGroup(maid_name);
//  auto metadata = CreateMaidManagerMetadata(maid);
//  GroupKey<MaidName> key(maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                         DataTagValue::kMaidValue);

//  EXPECT_THROW(maid_group_db.GetMetadata(maid_name), maidsafe_error);
//  EXPECT_THROW(maid_group_db.Commit(key, TestGroupDbActionPutValue()), maidsafe_error);

//  maid_group_db.AddGroup(maid_name, metadata);
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == metadata);
//  EXPECT_THROW(maid_group_db.AddGroup(maid_name, metadata), maidsafe_error);
//  EXPECT_TRUE(maid_group_db.GetContents(maid_name).kv_pairs.size() == 0U);
//  maid_group_db.DeleteGroup(maid_name);
//  EXPECT_THROW(maid_group_db.GetMetadata(maid_name), maidsafe_error);

//  maid_group_db.AddGroup(maid_name, metadata);
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == metadata);
//  EXPECT_THROW(maid_group_db.AddGroup(maid_name, metadata), maidsafe_error);
//  EXPECT_TRUE(maid_group_db.GetContents(maid_name).kv_pairs.size() == 0U);

//  GroupKey<MaidName> unknown_key(MaidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
//                                 Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                                 DataTagValue::kMaidValue);
//  EXPECT_THROW(maid_group_db.GetValue(unknown_key), maidsafe_error);

//  EXPECT_THROW(maid_group_db.GetValue(key), maidsafe_error);
//  EXPECT_THROW(maid_group_db.Commit(key, TestGroupDbActionModifyValue()), maidsafe_error);
//  EXPECT_THROW(maid_group_db.Commit(key, TestGroupDbActionDeleteValue()), maidsafe_error);
//  // Put
//  MaidManagerMetadata expected_metadata(metadata);
//  expected_metadata.PutData(100);
//  MaidManagerValue expected_value;
//  expected_value.Put(100);
//  maid_group_db.Commit(key, TestGroupDbActionPutValue());
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == expected_metadata);
//  EXPECT_TRUE(maid_group_db.GetValue(key) == expected_value);
//  EXPECT_TRUE((maid_group_db.GetContents(maid_name)).kv_pairs.size() == 1U);

//  // Modify and delete
//  expected_metadata.PutData(100);
//  expected_value.Put(100);
//  maid_group_db.Commit(key, TestGroupDbActionModifyValue());
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == expected_metadata);
//  EXPECT_TRUE(maid_group_db.GetValue(key) == expected_value);
//  EXPECT_TRUE((maid_group_db.GetContents(maid_name)).kv_pairs.size() == 1U);
//  expected_metadata.DeleteData(expected_value.Delete());
//  maid_group_db.Commit(key, TestGroupDbActionDeleteValue());
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == expected_metadata);
//  EXPECT_TRUE(maid_group_db.GetValue(key) == expected_value);
//  EXPECT_TRUE((maid_group_db.GetContents(maid_name)).kv_pairs.size() == 1U);

//  // Put many
//  std::vector<GroupKey<MaidName>> key_vector;
//  for (auto i(0); i < 1000; ++i) {
//    key_vector.push_back(GroupKey<MaidName>(
//        maid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()), DataTagValue::kMaidValue));
//  }
//  for (const auto& key_n : key_vector) {
//    expected_metadata.PutData(100);
//    maid_group_db.Commit(key_n, TestGroupDbActionPutValue());
//  }

//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == expected_metadata);

//  for (const auto& key_n : key_vector) {
//    EXPECT_TRUE(maid_group_db.GetValue(key_n) == expected_value);
//  }

//  auto contents(maid_group_db.GetContents(maid_name));
//  for (const auto& key_n : key_vector) {
//    auto itr = std::find_if(
//        contents.kv_pairs.begin(), contents.kv_pairs.end(),
//        [&key_n](const GroupDb<MaidManager>::KvPair& kv_pair) { return (kv_pair.first == key_n); });
//    EXPECT_TRUE(itr != contents.kv_pairs.end());
//  }

//  // Delete
//  expected_metadata.DeleteData(100);
//  maid_group_db.Commit(key, TestGroupDbActionDeleteValue());
//  EXPECT_TRUE(maid_group_db.GetMetadata(maid_name) == expected_metadata);
//  EXPECT_THROW(maid_group_db.GetValue(key), maidsafe_error);
//  maid_group_db.DeleteGroup(maid_name);
//  EXPECT_THROW(maid_group_db.GetMetadata(maid_name), maidsafe_error);
//  EXPECT_THROW(maid_group_db.GetContents(maid_name), maidsafe_error);
//}

//void RunPmidManagerGroupDbTest(GroupDb<PmidManager>& pmid_group_db) {
//  auto pmid(passport::CreatePmidAndSigner().first);
//  passport::PublicPmid::Name pmid_name(PmidName(pmid.name()));
//  EXPECT_THROW(pmid_group_db.GetMetadata(pmid_name), maidsafe_error);
//  pmid_group_db.DeleteGroup(pmid_name);
//  auto metadata = CreatePmidManagerValue(pmid_name);
//  GroupKey<PmidName> key(pmid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                         DataTagValue::kPmidValue);
//  EXPECT_THROW(pmid_group_db.Commit(key, TestPmidGroupDbActionDeleteValue()), maidsafe_error);
//  EXPECT_THROW(pmid_group_db.GetMetadata(pmid_name), maidsafe_error);

//  pmid_group_db.AddGroup(pmid_name, metadata);
//  EXPECT_TRUE(pmid_group_db.GetMetadata(pmid_name) == metadata);
//  EXPECT_THROW(pmid_group_db.AddGroup(pmid_name, metadata), maidsafe_error);
//  EXPECT_TRUE(pmid_group_db.GetContents(pmid_name).kv_pairs.size() == 0U);
//  pmid_group_db.DeleteGroup(pmid_name);
//  EXPECT_THROW(pmid_group_db.GetMetadata(pmid_name), maidsafe_error);
//  pmid_group_db.AddGroup(pmid_name, metadata);
//  EXPECT_TRUE(pmid_group_db.GetMetadata(pmid_name) == metadata);
//  EXPECT_THROW(pmid_group_db.AddGroup(pmid_name, metadata), maidsafe_error);
//  EXPECT_TRUE(pmid_group_db.GetContents(pmid_name).kv_pairs.size() == 0U);
//  GroupKey<PmidName> unknown_key(PmidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
//                                 Identity(NodeId(NodeId::IdType::kRandomId).string()),
//                                 DataTagValue::kPmidValue);
//  EXPECT_THROW(pmid_group_db.GetValue(unknown_key), maidsafe_error);
//  EXPECT_THROW(pmid_group_db.GetValue(key), maidsafe_error);
//  EXPECT_THROW(pmid_group_db.Commit(key, TestPmidGroupDbActionDeleteValue()), maidsafe_error);
//  pmid_group_db.Commit(key, TestPmidGroupDbActionPutValue());
//  pmid_group_db.Commit(key, TestPmidGroupDbActionDeleteValue());
//  // Put
//  PmidManagerValue expected_metadata(metadata);
//  expected_metadata.PutData(100);
//  PmidManagerValue expected_value(100);
//  pmid_group_db.Commit(key, TestPmidGroupDbActionPutValue());
//  EXPECT_TRUE(pmid_group_db.GetMetadata(pmid_name) == expected_metadata);
//  EXPECT_TRUE(pmid_group_db.GetValue(key) == expected_value);
//  EXPECT_TRUE((pmid_group_db.GetContents(pmid_name)).kv_pairs.size() == 1U);

//  // Put many
//  std::vector<GroupKey<PmidName>> key_vector;
//  for (auto i(0); i < 100; ++i) {
//    key_vector.push_back(GroupKey<PmidName>(
//        pmid_name, Identity(NodeId(NodeId::IdType::kRandomId).string()), DataTagValue::kPmidValue));
//  }
//  for (const auto& key_n : key_vector) {
//    expected_metadata.PutData(100);
//    pmid_group_db.Commit(key_n, TestPmidGroupDbActionPutValue());
//  }

//  EXPECT_TRUE(pmid_group_db.GetMetadata(pmid_name) == expected_metadata);

//  for (const auto& key_n : key_vector) {
//    EXPECT_TRUE(pmid_group_db.GetValue(key_n) == expected_value);
//  }
//  auto contents(pmid_group_db.GetContents(pmid_name));
//  for (const auto& key_n : key_vector) {
//    auto itr = std::find_if(
//        contents.kv_pairs.begin(), contents.kv_pairs.end(),
//        [&key_n](const GroupDb<PmidManager>::KvPair& kv_pair) { return (kv_pair.first == key_n); });
//    EXPECT_TRUE(itr != contents.kv_pairs.end());
//  }

//  // Delete
//  expected_metadata.DeleteData(100);
//  pmid_group_db.Commit(key, TestPmidGroupDbActionDeleteValue());
//  EXPECT_TRUE(pmid_group_db.GetMetadata(pmid_name) == expected_metadata);
//  EXPECT_THROW(pmid_group_db.GetValue(key), maidsafe_error);

//  // Delete group
//  pmid_group_db.DeleteGroup(pmid_name);
//  EXPECT_THROW(pmid_group_db.GetMetadata(pmid_name), maidsafe_error);
//  EXPECT_THROW(pmid_group_db.GetContents(pmid_name), maidsafe_error);
//}


//TEST(GroupDbTest, BEH_Constructor) {
//  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_GroupDbTest"));

//  GroupDb<MaidManager> maid_group_db(UniqueDbPath(*test_path));
//  GroupDb<PmidManager> pmid_group_db(UniqueDbPath(*test_path));
//}

//TEST(GroupDbTest, FUNC_MaidManagerGroupDb) {
//  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_GroupDbTest"));
//  GroupDb<MaidManager> maid_group_db(UniqueDbPath(*test_path));
//  for (auto i(0); i < 5; ++i)
//    RunMaidManagerGroupDbTest(maid_group_db);

//  RunDbTestInParallel(10, [&] { RunMaidManagerGroupDbTest(maid_group_db); });
//}

//TEST(GroupDbTest, FUNC_PmidManagerGroupDb) {
//  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_GroupDbTest"));
//  GroupDb<PmidManager> pmid_group_db(UniqueDbPath(*test_path));
//  for (auto i(0); i < 5; ++i)
//    RunPmidManagerGroupDbTest(pmid_group_db);

//  RunDbTestInParallel(10, [&] { RunPmidManagerGroupDbTest(pmid_group_db); });
//}

//TEST(GroupDbTest, BEH_TransferInfo) {
//  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_GroupDbTest"));
//  GroupDb<MaidManager> maid_group_db(UniqueDbPath(*test_path));
//  GroupDb<PmidManager> pmid_group_db(UniqueDbPath(*test_path));
//  std::shared_ptr<routing::CloseNodesChange> close_nodes_change;
//  maid_group_db.GetTransferInfo(close_nodes_change);
//  pmid_group_db.GetTransferInfo(close_nodes_change);
//}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
