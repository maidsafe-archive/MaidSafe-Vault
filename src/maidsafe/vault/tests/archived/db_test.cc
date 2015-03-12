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
    Key key(Identity(NodeId(RandomString(identity_size)).string()), DataTagValue::kMaidValue);
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
  Key key(Identity(NodeId(RandomString(identity_size)).string()), DataTagValue::kMaidValue);
  for (auto i(0); i != 100; ++i)
    DbTests(db, key);
  // TODO(Prakash): Extend to all data types
  PopulateDbValues(db, 1000);
  for (auto i(0); i != 100; ++i) {
    DbTests(db,
            Key(Identity(NodeId(RandomString(identity_size)).string()), DataTagValue::kMaidValue));
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

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
