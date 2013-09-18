///*  Copyright 2012 MaidSafe.net limited

//    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
//    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
//    licence you accepted on initial access to the Software (the "Licences").

//    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
//    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
//    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
//    available at: http://www.maidsafe.net/licenses

//    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
//    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
//    OF ANY KIND, either express or implied.

//    See the Licences for the specific language governing permissions and limitations relating to
//    use of the MaidSafe Software.                                                                 */

//#include <functional>
//#include <memory>

//#include "boost/filesystem/operations.hpp"
//#include "boost/filesystem/path.hpp"
//#include "boost/progress.hpp"

//#include "leveldb/db.h"
//#include "leveldb/options.h"

//#include "leveldb/status.h"

//#include "maidsafe/common/test.h"
//#include "maidsafe/common/utils.h"

//#include "maidsafe/passport/types.h"

//#include "maidsafe/vault/db.h"

//namespace maidsafe {
//namespace vault {
//namespace test {

//const uint64_t kValueSize(36);

//class DbTest : public testing::Test {
// public:
//  DbTest()
//      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
//        vault_root_directory_(*kTestRoot_ / RandomAlphaNumericString(8)) {
//    boost::filesystem::create_directory(vault_root_directory_);
//  }

//  DataNameVariant GetRandomKey() {
//    // Currently 15 types are defined, but...
//    uint32_t number_of_types = boost::mpl::size<typename DataNameVariant::types>::type::value,
//             type_number;
//    type_number = RandomUint32() % number_of_types;
//    switch (type_number) {
//      case  0: return passport::Anmid::Name();
//      case  1: return passport::Ansmid::Name();
//      case  2: return passport::Antmid::Name();
//      case  3: return passport::Anmaid::Name();
//      case  4: return passport::Maid::Name();
//      case  5: return passport::Pmid::Name();
//      case  6: return passport::Mid::Name();
//      case  7: return passport::Smid::Name();
//      case  8: return passport::Tmid::Name();
//      case  9: return passport::Anmpid::Name();
//      case 10: return passport::Mpid::Name();
//      case 11: return ImmutableData::Name();
//      case 12: return OwnerDirectory::Name();
//      case 13: return GroupDirectory::Name();
//      case 14: return WorldDirectory::Name();
//      // default:
//        // Throw something!
//      //  ;
//    }
//    return DataNameVariant();
//  }

//  struct GenerateKeyValuePair : public boost::static_visitor<NonEmptyString>
//  {
//    GenerateKeyValuePair() : size_(kValueSize) {}
//    explicit GenerateKeyValuePair(uint32_t size) : size_(size) {}

//    template<typename T>
//    NonEmptyString operator()(T& key)
//    {
//      NonEmptyString value = NonEmptyString(RandomAlphaNumericString(size_));
//      key.data = Identity(crypto::Hash<crypto::SHA512>(value));
//      return value;
//    }

//    uint32_t size_;
//  };

//  NonEmptyString GenerateKeyValueData(DbKey& key, uint32_t size) {
//    GenerateKeyValuePair generate_key_value_pair_(size);
//    DataNameVariant data_name_variant(key.name());
//    NonEmptyString result(boost::apply_visitor(generate_key_value_pair_, data_name_variant));
//    key = DbKey(data_name_variant);
//    return result;
//  }

//  NonEmptyString GenerateValue(uint32_t size = kValueSize) {
//    return NonEmptyString(RandomAlphaNumericString(size));
//  }

// protected:
//  const maidsafe::test::TestPath kTestRoot_;
//  boost::filesystem::path vault_root_directory_;
//};

//TEST_F(DbTest, BEH_Constructor) {
//  Db db;
//  AccountDb account_db(db);
//}

//TEST_F(DbTest, BEH_Poc) {
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
//        ASSERT_NE(std::find(nodes1.begin(), nodes1.end(), iter->value().ToString()), nodes1.end());
//        ++count;
//    }
//    ASSERT_EQ(10000, count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("2");
//         iter->Valid() && iter->key().ToString() < "3";
//         iter->Next()) {
//        ASSERT_NE(std::find(nodes2.begin(), nodes2.end(), iter->value().ToString()), nodes2.end());
//        ++count;
//    }
//    ASSERT_EQ(10000, count);
//    delete iter;
//  }
//  {
//    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
//    uint32_t count(0);
//    for (iter->Seek("3");
//         iter->Valid();
//         iter->Next()) {
//        ASSERT_NE(std::find(nodes3.begin(), nodes3.end(), iter->value().ToString()), nodes3.end());
//        ++count;
//    }
//    ASSERT_EQ(10000, count);
//    delete iter;
//  }
//}

//TEST_F(DbTest, BEH_GetSingleAccount) {
//  Db db;
//  AccountDb account_db(db);
//  std::vector<Db::KvPair> nodes;
//  for (uint32_t i = 0; i != 10000; ++i) {
//    DbKey key(GetRandomKey());
//    NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//    nodes.push_back(std::make_pair(key, value));
//  }
//  for (uint32_t i = 0; i != 10000; ++i)
//    EXPECT_NO_THROW(account_db.Put(std::make_pair(nodes[i].first, nodes[i].second)));
//  for (uint32_t i = 0; i != 10000; ++i) {
//    NonEmptyString value;
//    EXPECT_NO_THROW(value = account_db.Get(nodes[i].first));
//    EXPECT_EQ(nodes[i].second, value);
//  }
//}

//TEST_F(DbTest, BEH_DeleteSingleAccount) {
//  Db db;
//  AccountDb account_db(db);
//  std::vector<Db::KvPair> nodes;
//  for (uint32_t i = 0; i != 10000; ++i) {
//    DbKey key(GetRandomKey());
//    NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//    nodes.push_back(std::make_pair(key, value));
//  }
//  for (uint32_t i = 0; i != 10000; ++i)
//    EXPECT_NO_THROW(account_db.Put(std::make_pair(nodes[i].first, nodes[i].second)));
//  for (uint32_t i = 0; i != 10000; ++i)
//    EXPECT_EQ(nodes[i].second, account_db.Get(nodes[i].first));
//  for (uint32_t i = 0; i != 10000; ++i)
//    EXPECT_NO_THROW(account_db.Delete(nodes[i].first));
//  for (uint32_t i = 0; i != 10000; ++i)
//    EXPECT_THROW(account_db.Get(nodes[i].first), vault_error);
//}

//TEST_F(DbTest, BEH_GetMultipleAccounts) {
//  uint32_t accounts(RandomUint32() % 10);
//  std::vector<std::vector<Db::KvPair>> account_vector(accounts);
//  Db db;
//  std::vector<std::unique_ptr<AccountDb>> account_db_vector(accounts);
//  for (uint32_t i = 0; i != accounts; ++i) {
//    account_db_vector[i].reset(new AccountDb(db));
//  }
//  for (uint32_t i = 0; i != accounts; ++i) {
//    uint32_t entries(RandomUint32() % 10000);
//    for (uint32_t j = 0; j != entries; ++j) {
//      DbKey key(GetRandomKey());
//      NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//      account_vector[i].push_back(std::make_pair(key, value));
//    }
//  }
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_NO_THROW(account_db_vector[i]->Put(std::make_pair(account_vector[i][j].first,
//                                                       account_vector[i][j].second)));
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_EQ(account_vector[i][j].second, account_db_vector[i]->Get(account_vector[i][j].first));
//}

//TEST_F(DbTest, BEH_DeleteMultipleAccounts) {
//  uint32_t accounts(RandomUint32() % 10);
//  std::vector<std::vector<Db::KvPair>> account_vector(accounts);
//  Db db;
//  std::vector<std::unique_ptr<AccountDb>> account_db_vector(accounts);
//  for (uint32_t i = 0; i != accounts; ++i) {
//    account_db_vector[i].reset(new AccountDb(db));
//  }
//  for (uint32_t i = 0; i != accounts; ++i) {
//    uint32_t entries(RandomUint32() % 10000);
//    for (uint32_t j = 0; j != entries; ++j) {
//      DbKey key(GetRandomKey());
//      NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//      account_vector[i].push_back(std::make_pair(key, value));
//    }
//  }
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_NO_THROW(account_db_vector[i]->Put(std::make_pair(account_vector[i][j].first,
//                                                       account_vector[i][j].second)));
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_EQ(account_vector[i][j].second, account_db_vector[i]->Get(account_vector[i][j].first));
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_NO_THROW(account_db_vector[i]->Delete(account_vector[i][j].first));
//  for (uint32_t i = 0; i != accounts; ++i)
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j)
//      EXPECT_THROW(account_db_vector[i]->Get(account_vector[i][j].first), vault_error);
//}

//TEST_F(DbTest, BEH_AsyncGetPuts) {
//  std::mutex op_mutex, cond_mutex;
//  std::condition_variable cond_var;
//  std::vector<std::future<void>> async_ops;
//  uint32_t accounts(RandomUint32() % 10), expected_count(0), op_count(0);
//  Db db;
//  std::vector<std::vector<Db::KvPair>> account_vector(accounts);
//  std::vector<std::unique_ptr<AccountDb>> account_db_vector(accounts);
//  for (uint32_t i = 0; i != accounts; ++i) {
//    account_db_vector[i].reset(new AccountDb(db));
//  }
//  for (uint32_t i = 0; i != accounts; ++i) {
//    uint32_t entries(RandomUint32() % 1000);
//    expected_count += entries;
//    for (uint32_t j = 0; j != entries; ++j) {
//      DbKey key(GetRandomKey());
//      NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//      account_vector[i].push_back(std::make_pair(key, value));
//      //async_ops.push_back(std::async(
//      std::async(
//          std::launch::async,
//          [this, &account_db_vector, &account_vector, &op_count, &op_mutex, i, j] {
//              EXPECT_NO_THROW(account_db_vector[i]->Put(std::make_pair(account_vector[i][j].first,
//                                                               account_vector[i][j].second)));
//              {
//                std::lock_guard<std::mutex> lock(op_mutex);
//                ++op_count;
//              }
//          });
//          // }));
//    }
//  }
//  {
//    std::unique_lock<std::mutex> lock(cond_mutex);
//    bool result(cond_var.wait_for(lock, std::chrono::seconds(5),
//                                  [&]()->bool {
//                                    return op_count == expected_count;
//                                  }));
//    EXPECT_TRUE(result);
//    /*for (uint32_t i = 0; i != async_ops.size(); ++i)
//      EXPECT_NO_THROW(async_ops[i].get());
//    async_ops.clear();*/
//    op_count = 0;
//  }

//  for (uint32_t i = 0; i != accounts; ++i) {
//    for (uint32_t j = 0; j != account_vector[i].size(); ++j) {
//      //async_ops.push_back(std::async(
//      std::async(
//          std::launch::async,
//          [this, &account_db_vector, &account_vector, &op_count, &op_mutex, i, j] {
//              EXPECT_EQ(account_vector[i][j].second,
//                        account_db_vector[i]->Get(account_vector[i][j].first));
//              {
//                std::lock_guard<std::mutex> lock(op_mutex);
//                ++op_count;
//              }
//          });
//          //}));
//    }
//  }
//  {
//    std::unique_lock<std::mutex> lock(cond_mutex);
//    bool result(cond_var.wait_for(lock, std::chrono::seconds(5),
//                                  [&]()->bool {
//                                    return op_count == expected_count;
//                                  }));
//    EXPECT_TRUE(result);
//    /*for (uint32_t i = 0; i != async_ops.size(); ++i)
//      EXPECT_NO_THROW(async_ops[i].get());
//    async_ops.clear();*/
//  }
//}

//TEST_F(DbTest, BEH_ParallelAccountCreation) {
//  Db db;
//  for (uint32_t i = 0; i != 100; ++i) {
//    std::async(std::launch::async, [this, &db]() {
//        AccountDb account_db(db);
//        std::vector<Db::KvPair> nodes;
//        for (uint32_t i = 0; i != 100; ++i) {
//          DbKey key(GetRandomKey());
//          NonEmptyString value(GenerateKeyValueData(key, kValueSize));
//          nodes.push_back(std::make_pair(key, value));
//        }
//        for (uint32_t i = 0; i != 100; ++i)
//          EXPECT_NO_THROW(account_db.Put(std::make_pair(nodes[i].first, nodes[i].second)));
//        for (uint32_t i = 0; i != 100; ++i)
//          EXPECT_EQ(nodes[i].second, account_db.Get(nodes[i].first));
//    });
//  }
//}

//TEST_F(DbTest, BEH_PutSameKeyDifferentValue) {
//  Db db;
//  AccountDb account_db(db);
//  DbKey key(GetRandomKey());
//  NonEmptyString value(GenerateKeyValueData(key, kValueSize)), last_value;
//  const uint32_t entries(100);
//  for (uint32_t i = 0; i != entries; ++i) {
//    EXPECT_NO_THROW(account_db.Put(std::make_pair(key, value)));
//    if (i == entries - 1)
//      last_value = value;
//    else
//      value = GenerateValue(kValueSize);
//  }

//  EXPECT_EQ(last_value, account_db.Get(key));
//}

//}  // namespace test
//}  // namespace vault
//}  // namespace maidsafe
