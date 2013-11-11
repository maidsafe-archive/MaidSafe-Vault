/*  Copyright 2012 MaidSafe.net limited

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
    use of the MaidSafe Software.
*/

#include "maidsafe/common/test.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/unresolved_action.pb.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class PmidManagerServiceTest  : public testing::Test {
 public:
  PmidManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      pmid_manager_service_(pmid_, routing_) {}

 protected:
  passport::Maid MakeMaid() {
    passport::Anmaid anmaid;
    return passport::Maid(anmaid);
  }

  passport::Pmid MakePmid() { return passport::Pmid(MakeMaid()); }

  passport::PublicPmid MakePublicPmid() {
    passport::Pmid pmid(MakePmid());
    return passport::PublicPmid(pmid);
  }

  passport::Pmid pmid_;
  routing::Routing routing_;
  PmidManagerService pmid_manager_service_;
};

TEST_F(PmidManagerServiceTest, BEH_PutSynchroniseFromPmidManager) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionPmidManagerPut action_put(TEST_CHUNK_SIZE, nfs::MessageId(RandomInt32()));
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  auto group_source(CreateGroupSource(NodeId(pmid_name.value.string())));
  PmidManager::Key key(pmid_name, data.name(), ImmutableData::Tag::kValue);
  pmid_manager_service_.group_db_.AddGroup(pmid_name, PmidManagerMetadata());
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<PmidManager::UnresolvedPut>(key, action_put, group_source));
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedPut,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_puts_, group_unresolved_action,
      group_source);
  try {
    auto value(pmid_manager_service_.group_db_.GetValue(key));
    auto metadata(pmid_manager_service_.group_db_.GetMetadata(pmid_name));
    EXPECT_EQ(value.size(), TEST_CHUNK_SIZE);
    EXPECT_EQ(metadata.stored_total_size, TEST_CHUNK_SIZE);
    EXPECT_EQ(metadata.stored_count, 1);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(false);
  }
}

TEST_F(PmidManagerServiceTest, BEH_DeleteSynchroniseFromPmidManager) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionPmidManagerDelete action_delete;
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  auto group_source(CreateGroupSource(NodeId(pmid_name.value.string())));
  PmidManager::Key key(pmid_name, data.name(), ImmutableData::Tag::kValue);
  pmid_manager_service_.group_db_.AddGroup(pmid_name, PmidManagerMetadata());
  pmid_manager_service_.group_db_.Commit(
      key, ActionPmidManagerPut(TEST_CHUNK_SIZE, nfs::MessageId(RandomInt32())));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<PmidManager::UnresolvedDelete>(key, action_delete,
                                                                      group_source));
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedDelete,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_deletes_, group_unresolved_action,
      group_source);
  try {
    pmid_manager_service_.group_db_.GetValue(key);
    EXPECT_TRUE(false);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(true);
  }

  try {
    pmid_manager_service_.group_db_.GetMetadata(pmid_name);
    EXPECT_TRUE(false);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(true);
  }
}

TEST_F(PmidManagerServiceTest, BEH_PutRequestFromDataManager) {
  auto content(CreateContent<PutRequestFromDataManagerToPmidManager::Contents>());
  auto put_request(CreateMessage<PutRequestFromDataManagerToPmidManager>(content));
  routing::GroupSource group_source(
      routing::GroupId(NodeId(put_request.contents->name.raw_name.string())),
      routing::SingleId(NodeId(NodeId::kRandomId)));
  this->pmid_manager_service_.HandleMessage(put_request, group_source,
                                            routing::GroupId(this->routing_.kNodeId()));
  EXPECT_EQ(this->pmid_manager_service_.sync_puts_.GetUnresolvedActions().size(), 1);
}

TEST_F(PmidManagerServiceTest, BEH_PutFailureFromPmidNode) {
  PmidName pmid_name(Identity(RandomString(64)));
  PmidManagerMetadata metadata(pmid_name);
  metadata.claimed_available_size = TEST_CHUNK_SIZE * 100;
  pmid_manager_service_.group_db_.AddGroup(pmid_name, PmidManagerMetadata());
  auto content(CreateContent<PutFailureFromPmidNodeToPmidManager::Contents>());
  auto put_failure(CreateMessage<PutFailureFromPmidNodeToPmidManager>(content));
  SingleSendsToGroup(&pmid_manager_service_, put_failure,
                     routing::SingleSource(NodeId(NodeId::kRandomId)),
                     routing::GroupId(NodeId(pmid_name->string())));
  EXPECT_EQ(pmid_manager_service_.sync_deletes_.GetUnresolvedActions().size(), 1);
  try {
    auto metadata(pmid_manager_service_.group_db_.GetMetadata(pmid_name));
    EXPECT_EQ(metadata.claimed_available_size, 0);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(false);
  }
}

TEST_F(PmidManagerServiceTest, BEH_DeleterequestFromDataManager) {
  auto content(CreateContent<DeleteRequestFromDataManagerToPmidManager::Contents>());
  auto delete_request(CreateMessage<DeleteRequestFromDataManagerToPmidManager>(content));
  auto group_sources(CreateGroupSource(NodeId(content.raw_name.string())));
  NodeId pmid_node(NodeId::kRandomId);
  for (const auto& group_source : group_sources) {
    this->pmid_manager_service_.HandleMessage(delete_request, group_source,
                                              routing::GroupId(pmid_node));
  }
  EXPECT_EQ(this->pmid_manager_service_.sync_deletes_.GetUnresolvedActions().size(), 1);
}

TEST_F(PmidManagerServiceTest, BEH_GetPmidAccountRequestFromPmidNode) {
  auto content(CreateContent<GetPmidAccountRequestFromPmidNodeToPmidManager::Contents>());
  auto get_pmid_account_request(CreateMessage<GetPmidAccountRequestFromPmidNodeToPmidManager>(
                                    content));
  NodeId pmid_node(NodeId::kRandomId);
  this->pmid_manager_service_.HandleMessage(get_pmid_account_request,
                                            routing::SingleSource(pmid_node),
                                            routing::GroupId(pmid_node));
}

TEST_F(PmidManagerServiceTest, BEH_PmidHealthRequestFromMaidNode) {
  auto content(CreateContent<PmidHealthRequestFromMaidNodeToPmidManager::Contents>());
  auto get_pmid_account_request(CreateMessage<PmidHealthRequestFromMaidNodeToPmidManager>(content));
  NodeId pmid_node(NodeId::kRandomId), maid_node(NodeId::kRandomId);
  this->pmid_manager_service_.HandleMessage(get_pmid_account_request,
                                            routing::SingleSource(maid_node),
                                            routing::GroupId(pmid_node));
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
