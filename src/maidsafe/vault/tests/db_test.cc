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

#include "maidsafe/vault/db.h"

#include "leveldb/db.h"
#include "leveldb/options.h"

#include "leveldb/status.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/key.h"
#include "maidsafe/vault/version_handler/value.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

struct TestDbValue {
  TestDbValue() : value("original_value") {}
  TestDbValue(TestDbValue&& other) : value(std::move(other.value)) {}
  explicit TestDbValue(const std::string& serialised_value) : value(serialised_value) {}
  std::string Serialise() const { return value; }
  std::string value;

 private:
  TestDbValue(const TestDbValue&);
};

// change value
struct TestDbActionModifyValue {
  explicit TestDbActionModifyValue(const std::string& value) : kValue(value) {}
  detail::DbAction operator()(std::unique_ptr<TestDbValue>& value) {
    if (value) {
      value->value = "modified_value";
      return detail::DbAction::kPut;
    }
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  }
  const std::string kValue;
};

// put value
struct TestDbActionPutValue {
  explicit TestDbActionPutValue(const std::string& value) : kValue(value) {}
  detail::DbAction operator()(std::unique_ptr<TestDbValue>& value) {
    if (!value)
      value.reset(new TestDbValue());
    value->value = kValue;
    return detail::DbAction::kPut;
  }
  const std::string kValue;
};

// delete value
struct TestDbActionDeleteValue {
  detail::DbAction operator()(std::unique_ptr<TestDbValue>& value) {
    if (!value)
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
    return detail::DbAction::kDelete;
  }
};

template <typename Key, typename Value>
void PopulateDbValues(Db<Key, Value>& db, const int& count) {
  for (auto i(0); i != count; ++i) {
    // need Random type
    Key key(Identity(NodeId(NodeId::IdType::kRandomId).string()), DataTagValue::kMaidValue);
    db.Commit(key, TestDbActionPutValue("new_value"));
    EXPECT_TRUE(db.Get(key).value == "new_value");
  }
}

template <typename Key, typename Value>
void DbTests(Db<Key, Value>& db, const Key& key) {
  EXPECT_THROW(db.Get(key), maidsafe_error);
  db.Commit(key, TestDbActionPutValue("new_value"));
  EXPECT_TRUE(db.Get(key).value == "new_value");
  db.Commit(key, TestDbActionModifyValue("modified_value"));
  EXPECT_TRUE(db.Get(key).value == "modified_value");
  db.Commit(key, TestDbActionPutValue("new_value"));
  EXPECT_TRUE(db.Get(key).value == "new_value");
  db.Commit(key, TestDbActionDeleteValue());
  EXPECT_THROW(db.Get(key), maidsafe_error);
  EXPECT_THROW(db.Commit(key, TestDbActionModifyValue("modified_value")), maidsafe_error);
  EXPECT_THROW(db.Get(key), maidsafe_error);
}

TEST(DbTest, BEH_DbConstructor) {
  maidsafe::test::TestPath test_path1(maidsafe::test::CreateTestPath("MaidSafe_Test_DbTest1"));
  Db<Key, DataManagerValue> data_manager_db(UniqueDbPath(*test_path1));
  maidsafe::test::TestPath test_path2(maidsafe::test::CreateTestPath("MaidSafe_Test_DbTest2"));
  Db<Key, VersionHandlerValue> version_handler_db(UniqueDbPath(*test_path2));
}

TEST(DbTest, BEH_DbCommit) {
  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_DbTest"));
  Db<Key, TestDbValue> db(UniqueDbPath(*test_path));
  Key key(Identity(NodeId(NodeId::IdType::kRandomId).string()), DataTagValue::kMaidValue);
  for (auto i(0); i != 100; ++i)
    DbTests(db, key);
  // TODO(Prakash): Extend to all data types
  PopulateDbValues(db, 1000);
  for (auto i(0); i != 100; ++i) {
    DbTests(db,
            Key(Identity(NodeId(NodeId::IdType::kRandomId).string()), DataTagValue::kMaidValue));
  }
}

TEST(DbTest, BEH_DbTransferInfo) {
  maidsafe::test::TestPath test_path1(maidsafe::test::CreateTestPath("MaidSafe_Test_DbTest1"));
  Db<Key, DataManagerValue> data_manager_db(UniqueDbPath(*test_path1));
  maidsafe::test::TestPath test_path2(maidsafe::test::CreateTestPath("MaidSafe_Test_DbTest2"));
  Db<Key, VersionHandlerValue> version_handler_db(UniqueDbPath(*test_path2));
  std::shared_ptr<routing::CloseNodesChange> close_nodes_change;
  data_manager_db.GetTransferInfo(close_nodes_change);
  version_handler_db.GetTransferInfo(close_nodes_change);
}

// parallel test



// TEST("Db Poc", "[Db][Unit]") {
// const maidsafe::test::TestPath kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault"));
//  boost::filesystem::path vault_root_directory_(*kTestRoot_ / RandomAlphaNumericString(8));
//  leveldb::DB* db;
//  leveldb::Options options;
//  options.create_if_missing = true;
//  options.error_if_exists = true;
//  leveldb::Status status(leveldb::DB::Open(options, vault_root_directory_.string(), &db));
//  if (!status.ok())
//    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
//  std::unique_ptr<leveldb::DB> leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
//  assert(leveldb_);
//  std::vector<std::string> nodes1, nodes2, nodes3;
//  for (auto i(0U); i != 10000; ++i) {
//    nodes1.push_back("1" + NodeId(NodeId::IdType::kRandomId).string());
//    nodes2.push_back("2" + NodeId(NodeId::IdType::kRandomId).string());
//    nodes3.push_back("3" + NodeId(NodeId::IdType::kRandomId).string());

//    leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(), nodes1.back(), nodes1.back()));
//    if (!status.ok())
//      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
//    status = leveldb_->Put(leveldb::WriteOptions(), nodes2.back(), nodes2.back());
//    if (!status.ok())
//      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
//    status = leveldb_->Put(leveldb::WriteOptions(), nodes3.back(), nodes3.back());
//    if (!status.ok())
//      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
//  }

//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("1");
//         iter->Valid() && iter->key().ToString() < "2";
//         iter->Next()) {
//       ASSERT_TRUE(std::find(nodes1.begin(), nodes1.end(), iter->value().ToString()) !=
// nodes1.end());
//        ++count;
//    }
//    ASSERT_TRUE(10000 == count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("2");
//         iter->Valid() && iter->key().ToString() < "3";
//         iter->Next()) {
//       ASSERT_TRUE(std::find(nodes2.begin(), nodes2.end(), iter->value().ToString()) !=
// nodes2.end());
//        ++count;
//    }
//    ASSERT_TRUE(10000 == count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("3");
//         iter->Valid();
//         iter->Next()) {
//       ASSERT_TRUE(std::find(nodes3.begin(), nodes3.end(), iter->value().ToString()) !=
// nodes3.end());
//        ++count;
//    }
//    ASSERT_TRUE(10000 == count);
//    delete iter;
//  }
// }

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
