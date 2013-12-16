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
#include "maidsafe/vault/version_handler/key.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

namespace test {

//DataNameVariant GetRandomKey() {
//  // Currently 15 types are defined, but...
//  uint32_t number_of_types = boost::mpl::size<typename DataNameVariant::types>::type::value,
//           type_number;
//  std::cout << number_of_types;
//  type_number = RandomUint32() % number_of_types;
//  switch (type_number) {
//    case  0: return passport::Anmid::Name();
////    case  1: return passport::Ansmid::Name();
////    case  2: return passport::Antmid::Name();
////    case  3: return passport::Anmaid::Name();
////    case  4: return passport::Maid::Name();
////    case  5: return passport::Pmid::Name();
////    case  6: return passport::Mid::Name();
////    case  7: return passport::Smid::Name();
////    case  8: return passport::Tmid::Name();
////    case  9: return passport::Anmpid::Name();
////    case 10: return passport::Mpid::Name();
////    case 11: return ImmutableData::Name();
////    case 12: return OwnerDirectory::Name();
////    case 13: return GroupDirectory::Name();
////    case 14: return WorldDirectory::Name();
//    default:
//      return DataNameVariant();
//  }
//}

struct TestDbValue {
  TestDbValue() : value("original_value") {}
  TestDbValue(TestDbValue&& other) : value(std::move(other.value)) {}
  TestDbValue(const std::string& serialised_value) : value(serialised_value) {}
  std::string Serialise() const { return value; }
  std::string value;

 private:
  TestDbValue(const TestDbValue&);
};

// change value
struct TestDbActionModifyValue {
  TestDbActionModifyValue(const std::string& value) : kValue(value) {}
  detail::DbAction operator ()(std::unique_ptr<TestDbValue>& value) {
    if (value) {
      value->value = "modified_value";
      return detail::DbAction::kPut;
    }
    ThrowError(CommonErrors::no_such_element);
    return detail::DbAction::kDelete;
  }
  const std::string kValue;
};

// put value
struct TestDbActionPutValue {
  TestDbActionPutValue(const std::string& value) : kValue(value) {}
  detail::DbAction operator ()(std::unique_ptr<TestDbValue>& value) {
    if (!value)
      value.reset(new TestDbValue());
    value->value = kValue;
    return detail::DbAction::kPut;
  }
  const std::string kValue;
};

// delete value
struct TestDbActionDeleteValue {
detail::DbAction operator ()(std::unique_ptr<TestDbValue>& value) {
  if (!value)
    ThrowError(CommonErrors::no_such_element);
   return detail::DbAction::kDelete;
  }
};

template <typename Key, typename Value>
void PopulateDbValues(Db<Key, Value>& db, const int& count) {
  for (auto i(0); i != count; ++i) {
    Key key(Identity(NodeId(NodeId::kRandomId).string()), DataTagValue::kMaidValue); // need Random type
    db.Commit(key, TestDbActionPutValue("new_value"));
    CHECK(db.Get(key).value == "new_value");
  }
}

template <typename Key, typename Value>
void DbTests(Db<Key, Value>& db, const Key& key) {
  CHECK_THROWS_AS(db.Get(key), maidsafe_error);
  db.Commit(key, TestDbActionPutValue("new_value"));
  CHECK(db.Get(key).value == "new_value");
  db.Commit(key, TestDbActionModifyValue("modified_value"));
  CHECK(db.Get(key).value == "modified_value");
  db.Commit(key, TestDbActionPutValue("new_value"));
  CHECK(db.Get(key).value == "new_value");
  db.Commit(key, TestDbActionDeleteValue());
  CHECK_THROWS_AS(db.Get(key), maidsafe_error);
  CHECK_THROWS_AS(db.Commit(key, TestDbActionModifyValue("modified_value")), maidsafe_error);
  CHECK_THROWS_AS(db.Get(key), maidsafe_error);
}

TEST_CASE("Db constructor", "[Db][Unit]") {
  Db<Key, DataManagerValue> data_manager_db;
  Db<VersionHandlerKey, VersionHandlerValue> version_handler_db;
}

TEST_CASE("Db commit", "[Db][Unit]") {
  Db<Key, TestDbValue> db;
  Key key(Identity(NodeId(NodeId::kRandomId).string()), DataTagValue::kMaidValue);
  for (auto i(0); i != 100; ++i)
    DbTests(db, key);
  // TODO (Prakash) Extend to all data types
  PopulateDbValues(db, 10000);
  for (auto i(0); i != 100; ++i) {
    DbTests(db, Key(Identity(NodeId(NodeId::kRandomId).string()), DataTagValue::kMaidValue));
  }
}

TEST_CASE("Db transfer info", "[Db][Unit]") {
  Db<Key, DataManagerValue> data_manager_db;
  Db<VersionHandlerKey, VersionHandlerValue> version_handler_db;
  std::shared_ptr<routing::MatrixChange> matrix_change;
  data_manager_db.GetTransferInfo(matrix_change);
  version_handler_db.GetTransferInfo(matrix_change);
}

// parallel test



//TEST_CASE("Db Poc", "[Db][Unit]") {
//  const maidsafe::test::TestPath kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault"));
//  boost::filesystem::path vault_root_directory_(*kTestRoot_ / RandomAlphaNumericString(8));
//  leveldb::DB* db;
//  leveldb::Options options;
//  options.create_if_missing = true;
//  options.error_if_exists = true;
//  leveldb::Status status(leveldb::DB::Open(options, vault_root_directory_.string(), &db));
//  if (!status.ok())
//    ThrowError(VaultErrors::failed_to_handle_request);
//  std::unique_ptr<leveldb::DB> leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
//  assert(leveldb_);
//  std::vector<std::string> nodes1, nodes2, nodes3;
//  for (auto i(0U); i != 10000; ++i) {
//    nodes1.push_back("1" + NodeId(NodeId::kRandomId).string());
//    nodes2.push_back("2" + NodeId(NodeId::kRandomId).string());
//    nodes3.push_back("3" + NodeId(NodeId::kRandomId).string());

//    leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(), nodes1.back(), nodes1.back()));
//    if (!status.ok())
//      ThrowError(VaultErrors::failed_to_handle_request);
//    status = leveldb_->Put(leveldb::WriteOptions(), nodes2.back(), nodes2.back());
//    if (!status.ok())
//      ThrowError(VaultErrors::failed_to_handle_request);
//    status = leveldb_->Put(leveldb::WriteOptions(), nodes3.back(), nodes3.back());
//    if (!status.ok())
//      ThrowError(VaultErrors::failed_to_handle_request);
//  }

//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("1");
//         iter->Valid() && iter->key().ToString() < "2";
//         iter->Next()) {
//        REQUIRE(std::find(nodes1.begin(), nodes1.end(), iter->value().ToString()) != nodes1.end());
//        ++count;
//    }
//    REQUIRE(10000 == count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("2");
//         iter->Valid() && iter->key().ToString() < "3";
//         iter->Next()) {
//        REQUIRE(std::find(nodes2.begin(), nodes2.end(), iter->value().ToString()) != nodes2.end());
//        ++count;
//    }
//    REQUIRE(10000 == count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("3");
//         iter->Valid();
//         iter->Next()) {
//        REQUIRE(std::find(nodes3.begin(), nodes3.end(), iter->value().ToString()) != nodes3.end());
//        ++count;
//    }
//    REQUIRE(10000 == count);
//    delete iter;
//  }
//}

}  // test

}  // namespace vault

}  // namespace maidsafe
