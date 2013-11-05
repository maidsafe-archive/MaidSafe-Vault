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

#include "boost/progress.hpp"

#include "leveldb/db.h"
#include "leveldb/options.h"

#include "leveldb/status.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/version_handler/key.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

namespace test {

//TODO(Prakash) to be moved to test utility
passport::Maid MakeMaid() {
  passport::Anmaid anmaid;
  return passport::Maid(anmaid);
}

passport::Pmid MakePmid() {
  return passport::Pmid(MakeMaid());
}

PmidManagerMetadata CreatePmidManagerMetadata(const PmidName& pmid_name) {
  PmidManagerMetadata metadata(pmid_name);
  metadata.SetAvailableSize(1000000);
  return metadata;
}

MaidManagerMetadata CreateMaidManagerMetadata(const passport::Maid& maid) {

  std::vector<PmidTotals> pmid_totals_vector;
  for (auto i(0); i != 1; ++i) {
    auto pmid(MakePmid());
    auto pmid_metadata(CreatePmidManagerMetadata(PmidName(pmid.name())));
    nfs_vault::PmidRegistration pmid_registration(maid, pmid, false);
    PmidTotals pmid_totals(pmid_registration.Serialise(), pmid_metadata);
    pmid_totals_vector.push_back(pmid_totals);
  }
  return MaidManagerMetadata(0, pmid_totals_vector);
}

// update metadata only

// change value
struct TestGroupDbActionModifyValue {
  detail::DbAction operator()(MaidManagerMetadata& metadata,
                              std::unique_ptr<MaidManagerValue>& value) {
    if (value) {
      value->Put(100);
      metadata.PutData(100);
      return detail::DbAction::kPut;
    }
      ThrowError(CommonErrors::no_such_element);
      return detail::DbAction::kDelete;
    }
  };

// put value
struct TestGroupDbActionPutValue {
  detail::DbAction operator()(MaidManagerMetadata& metadata,
                              std::unique_ptr<MaidManagerValue>& value) {
    if (!value)
      value.reset(new MaidManagerValue());
    value->Put(100);
    metadata.PutData(100);
    return detail::DbAction::kPut;
  }
};

// delete value
struct TestGroupDbActionDeleteValue {
  detail::DbAction operator()(MaidManagerMetadata& metadata,
                              std::unique_ptr<MaidManagerValue>& value) {
    if (!value)
      ThrowError(CommonErrors::no_such_element);

    metadata.DeleteData(value->Delete());
    assert(value->count() >= 0);
    if (value->count() == 0)
      return detail::DbAction::kDelete;
    else
      return detail::DbAction::kPut;
  }
};

TEST_CASE("GroupDb constructor", "[GroupDb][Unit]") {
  GroupDb<MaidManager> maid_group_db;
  GroupDb<PmidManager> pmid_group_db;
}

TEST_CASE("GroupDb commit", "[GroupDb][Unit]") {
  GroupDb<MaidManager> maid_group_db;
  auto maid(MakeMaid());
  passport::PublicMaid::Name maid_name(MaidName(maid.name()));
  CHECK_THROWS_AS(maid_group_db.GetMetadata(maid_name), maidsafe_error);
  maid_group_db.DeleteGroup(maid_name);
  auto metadata = CreateMaidManagerMetadata(maid);
  maid_group_db.AddGroup(maid_name, metadata);
  CHECK(maid_group_db.GetMetadata(maid_name) == metadata);
  CHECK_THROWS_AS(maid_group_db.AddGroup(maid_name, metadata), maidsafe_error);
  CHECK(maid_group_db.GetContents(maid_name).kv_pair.size() == 0U);
  maid_group_db.DeleteGroup(maid_name);
  CHECK_THROWS_AS(maid_group_db.GetMetadata(maid_name), maidsafe_error);

  maid_group_db.AddGroup(maid_name, metadata);
  CHECK(maid_group_db.GetMetadata(maid_name) == metadata);
  CHECK_THROWS_AS(maid_group_db.AddGroup(maid_name, metadata), maidsafe_error);
  CHECK(maid_group_db.GetContents(maid_name).kv_pair.size() == 0U);

  GroupKey<MaidName> unknown_key(MaidName(Identity(NodeId(NodeId::kRandomId).string())),
                                 Identity(NodeId(NodeId::kRandomId).string()),
                                 DataTagValue::kMaidValue);
  CHECK_THROWS_AS(maid_group_db.GetValue(unknown_key), maidsafe_error);

  GroupKey<MaidName> key(maid_name, Identity(NodeId(NodeId::kRandomId).string()),
                         DataTagValue::kMaidValue);
  CHECK_THROWS_AS(maid_group_db.GetValue(key), maidsafe_error);
  CHECK_THROWS_AS(maid_group_db.Commit(key, TestGroupDbActionModifyValue()), maidsafe_error);
  CHECK_THROWS_AS(maid_group_db.Commit(key, TestGroupDbActionDeleteValue()), maidsafe_error);

  MaidManagerMetadata expected_metadata(metadata);
  expected_metadata.PutData(100);
  MaidManagerValue expected_value;
  expected_value.Put(100);
  maid_group_db.Commit(key, TestGroupDbActionPutValue());
  CHECK(maid_group_db.GetMetadata(maid_name) == expected_metadata);
  CHECK(maid_group_db.GetValue(key) == expected_value);
  maid_group_db.GetContents(maid_name);
  CHECK((maid_group_db.GetContents(maid_name)).kv_pair.size() == 1U);
  expected_metadata.PutData(100);
  expected_value.Put(100);
  maid_group_db.Commit(key, TestGroupDbActionModifyValue());
  CHECK(maid_group_db.GetMetadata(maid_name) == expected_metadata);
  CHECK(maid_group_db.GetValue(key) == expected_value);
  CHECK((maid_group_db.GetContents(maid_name)).kv_pair.size() == 1U);
  expected_metadata.DeleteData(expected_value.Delete());
  maid_group_db.Commit(key, TestGroupDbActionDeleteValue());
  CHECK(maid_group_db.GetMetadata(maid_name) == expected_metadata);
  CHECK(maid_group_db.GetValue(key) == expected_value);
  CHECK((maid_group_db.GetContents(maid_name)).kv_pair.size() == 1U);
  expected_metadata.DeleteData(100);
  maid_group_db.Commit(key, TestGroupDbActionDeleteValue());
  CHECK(maid_group_db.GetMetadata(maid_name) == expected_metadata);
  CHECK(maid_group_db.GetContents(maid_name).kv_pair.size() == 0U);
  CHECK_THROWS_AS(maid_group_db.GetValue(key), maidsafe_error);
  maid_group_db.DeleteGroup(maid_name);
  CHECK_THROWS_AS(maid_group_db.GetMetadata(maid_name), maidsafe_error);
  CHECK_THROWS_AS(maid_group_db.GetContents(maid_name), maidsafe_error);
}

}  // test

}  // namespace vault

}  // namespace maidsafe
