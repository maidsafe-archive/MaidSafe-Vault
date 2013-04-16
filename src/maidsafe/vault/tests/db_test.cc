/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/db.h"

#include <functional>
#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"


#include "leveldb/db.h"
#include "leveldb/options.h"

#include "leveldb/status.h"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"


namespace maidsafe {

namespace vault {

namespace test {

class DbTest : public testing::Test {
 public:
  DbTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_directory_(*kTestRoot_ / RandomAlphaNumericString(8)) {
    boost::filesystem::create_directory(vault_root_directory_);
  }

 protected:

  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_directory_;
};

TEST_F(DbTest, BEH_Constructor) {
  Db db(vault_root_directory_);
}

//TODO to be replaced by maidsafe api level test
TEST_F(DbTest, BEH_Poc) {
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, vault_root_directory_.string(), &db));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  std::unique_ptr<leveldb::DB> leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
  assert(leveldb_);
  std::vector<std::string> nodes1, nodes2, nodes3;
  for (auto i(0U); i != 10000; ++i) {
    nodes1.push_back("1" + NodeId(NodeId::kRandomId).string());
    nodes2.push_back("2" + NodeId(NodeId::kRandomId).string());
    nodes3.push_back("3" + NodeId(NodeId::kRandomId).string());

    leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(), nodes1.back(), nodes1.back()));
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
    status = leveldb_->Put(leveldb::WriteOptions(), nodes2.back(), nodes2.back());
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
    status = leveldb_->Put(leveldb::WriteOptions(), nodes3.back(), nodes3.back());
    if (!status.ok())
      ThrowError(VaultErrors::failed_to_handle_request);
  }

  {
    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
    uint32_t count(0);
    for (iter->Seek("1");
         iter->Valid() && iter->key().ToString() < "2";
         iter->Next()) {
        ASSERT_NE(std::find(nodes1.begin(), nodes1.end(), iter->value().ToString()), nodes1.end());
        ++count;
    }
    ASSERT_EQ(10000, count);
    delete iter;
  }
  {
    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
    uint32_t count(0);
    for (iter->Seek("2");
         iter->Valid() && iter->key().ToString() < "3";
         iter->Next()) {
        ASSERT_NE(std::find(nodes2.begin(), nodes2.end(), iter->value().ToString()), nodes2.end());
        ++count;
    }
    ASSERT_EQ(10000, count);
    delete iter;
  }
  {
    leveldb::Iterator* iter = leveldb_->NewIterator(leveldb::ReadOptions());
    uint32_t count(0);
    for (iter->Seek("3");
         iter->Valid();
         iter->Next()) {
        ASSERT_NE(std::find(nodes3.begin(), nodes3.end(), iter->value().ToString()), nodes3.end());
        ++count;
    }
    ASSERT_EQ(10000, count);
    delete iter;
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
