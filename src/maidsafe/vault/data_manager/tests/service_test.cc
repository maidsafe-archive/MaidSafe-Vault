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
#include "maidsafe/common/asio_service.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {


class DataManagerServiceTest  : public testing::Test {
 public:
  DataManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      data_getter_(asio_service_, routing_, std::vector<passport::PublicPmid>()),
      data_manager_service_(pmid_, routing_, data_getter_),
      asio_service_(2) {}

 protected:
  passport::Pmid pmid_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  DataManagerService data_manager_service_;
  AsioService asio_service_;
};

TEST_F(DataManagerServiceTest, BEH_PutRequestFromMaidManager) {
  NodeId maid_node_id(NodeId::kRandomId), data_name_id;
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.data.name.raw_name.string());
  auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  GroupSendToGroup(&data_manager_service_, put_request, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_puts_.GetUnresolvedActions().size(), 0);
}

TEST_F(DataManagerServiceTest, BEH_PutResponseFromPmidManager) {
  NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.data.name.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_response(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  GroupSendToGroup(&data_manager_service_, put_response, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_add_pmids_.GetUnresolvedActions().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_PutFailureFromPmidManager) {
  NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
  auto content(CreateContent<PutFailureFromPmidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.name.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_failure(CreateMessage<PutFailureFromPmidManagerToDataManager>(content));
  GroupSendToGroup(&data_manager_service_, put_failure, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_remove_pmids_.GetUnresolvedActions().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromMaidNode) {
  NodeId data_name_id, maid_node_id(NodeId::kRandomId);
  auto content(CreateContent<nfs::GetRequestFromMaidNodeToDataManager::Contents>());
  data_name_id = NodeId(content.raw_name.string());
  auto get_request(CreateMessage<nfs::GetRequestFromMaidNodeToDataManager>(content));
  this->data_manager_service_.HandleMessage(get_request, routing::SingleSource(maid_node_id),
                                            routing::GroupId(data_name_id));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromDataGetter) {
  NodeId data_name_id, maid_node_id(NodeId::kRandomId);
  auto content(CreateContent<nfs::GetRequestFromDataGetterToDataManager::Contents>());
  data_name_id = NodeId(content.raw_name.string());
  auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(content));
  this->data_manager_service_.HandleMessage(get_request, routing::SingleSource(maid_node_id),
                                            routing::GroupId(data_name_id));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_GetResponseFromPmidNode) {
  NodeId pmid_node_id(NodeId::kRandomId);
  auto content(CreateContent<GetResponseFromPmidNodeToDataManager::Contents>());
  auto get_response(CreateMessage<GetResponseFromPmidNodeToDataManager>(content));
  this->data_manager_service_.HandleMessage(get_response, routing::SingleSource(pmid_node_id),
                                            routing::SingleId(routing_.kNodeId()));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_PutToCacheFromDataManager) {
  NodeId data_manager_id(NodeId::kRandomId);
  auto content(CreateContent<PutToCacheFromDataManagerToDataManager::Contents>());
  auto put_to_cache(CreateMessage<PutToCacheFromDataManagerToDataManager>(content));
  this->data_manager_service_.HandleMessage(put_to_cache,routing::SingleSource(data_manager_id),
                                            routing::SingleId(routing_.kNodeId()));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_GetFromCacheFromDataManager) {
  NodeId data_manager_id(NodeId::kRandomId);
  auto content(CreateContent<GetFromCacheFromDataManagerToDataManager::Contents>());
  auto get_from_cache(CreateMessage<GetFromCacheFromDataManagerToDataManager>(content));
  this->data_manager_service_.HandleMessage(get_from_cache, routing::SingleSource(data_manager_id),
                                            routing::GroupId(NodeId(content.raw_name.string())));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_GetCachedResponseFromCacheHandler) {
  NodeId cache_handler_id(NodeId::kRandomId);
  auto content(CreateContent<GetCachedResponseFromCacheHandlerToDataManager::Contents>());
  auto get_cache_response(CreateMessage<GetCachedResponseFromCacheHandlerToDataManager>(content));
  this->data_manager_service_.HandleMessage(get_cache_response,
                                            routing::SingleSource(cache_handler_id),
                                            routing::SingleId(routing_.kNodeId()));
  // TO BE CONTINUED
}

TEST_F(DataManagerServiceTest, BEH_DeleteRequestFromMaidManager) {
  NodeId maid_node_id(NodeId::kRandomId);
  auto content(CreateContent<DeleteRequestFromMaidManagerToDataManager::Contents>());
  auto delete_request(CreateMessage<DeleteRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  GroupSendToGroup(&data_manager_service_, delete_request, group_source,
                   routing::GroupId(NodeId(content.raw_name.string())));
  EXPECT_EQ(this->data_manager_service_.sync_deletes_.GetUnresolvedActions().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_PutSynchroniseFromDataManager) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  auto group_source(CreateGroupSource(data.name()));
  DataManager::Key key(data.name());
  data_manager_service_.db_.Commit(key, ActionDataManagerAddPmid(pmid_name, TEST_CHUNK_SIZE));
  EXPECT_EQ(data_manager_service_.db_.Get(key).Subscribers(), 1);
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<DataManager::UnresolvedPut>(key, action_put, group_source));
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedPut,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_puts_, group_unresolved_action,
      group_source);
  EXPECT_EQ(data_manager_service_.db_.Get(key).Subscribers(), 2);
}

TEST_F(DataManagerServiceTest, BEH_DeleteSynchroniseFromDataManager) {
  // store key value in db
  PmidName pmid_name(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  DataManager::Key key(data.name());
  data_manager_service_.db_.Commit(key, ActionDataManagerAddPmid(pmid_name, TEST_CHUNK_SIZE));
  EXPECT_EQ(data_manager_service_.db_.Get(key).Subscribers(), 1);
  // key value is in db
  ActionDataManagerDelete action_delete;
  auto group_source(CreateGroupSource(data.name()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<DataManager::UnresolvedDelete>(key, action_delete,
                                                                      group_source));
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedDelete,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_deletes_, group_unresolved_action,
      group_source);
  try {
    data_manager_service_.db_.Get(key);
    EXPECT_TRUE(false);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(true);
  }
}

TEST_F(DataManagerServiceTest, BEH_AddPmidSynchroniseFromDataManager) {
  // check key value is not in db
  PmidName pmid_name(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  DataManager::Key key(data.name());
  try {
    data_manager_service_.db_.Get(key);
    EXPECT_TRUE(false);
  }
  catch (const maidsafe_error& /*error*/) {
    EXPECT_TRUE(true);
  }

  // Sync AddPmid
  ActionDataManagerAddPmid action_add_pmid(pmid_name, TEST_CHUNK_SIZE);
  auto group_source(CreateGroupSource(data.name()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<DataManager::UnresolvedAddPmid>(key, action_add_pmid,
                                                                       group_source));
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedAddPmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_add_pmids_, group_unresolved_action,
      group_source);
  EXPECT_EQ(data_manager_service_.db_.Get(key).Subscribers(), 1);
}

TEST_F(DataManagerServiceTest, BEH_RemovePmidSynchroniseFromDataManager) {
  // store key value in db
  PmidName pmid_name_one(Identity(RandomString(64))), pmid_name_two(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(TEST_CHUNK_SIZE)));
  DataManager::Key key(data.name());
  data_manager_service_.db_.Commit(key, ActionDataManagerAddPmid(pmid_name_one, TEST_CHUNK_SIZE));
  data_manager_service_.db_.Commit(key, ActionDataManagerAddPmid(pmid_name_two, TEST_CHUNK_SIZE));
  auto value(data_manager_service_.db_.Get(key));
  EXPECT_EQ(value.Subscribers(), 1);
  EXPECT_EQ(value.AllPmids().size(), 2);

  // Sync remove pmid
  ActionDataManagerRemovePmid action_remove_pmid(pmid_name_two);
  auto group_source(CreateGroupSource(data.name()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<DataManager::UnresolvedRemovePmid>(key, action_remove_pmid,
                                                                          group_source));
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedRemovePmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_remove_pmids_, group_unresolved_action,
      group_source);
  EXPECT_EQ(data_manager_service_.db_.Get(key).AllPmids().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_NodeDownSynchroniseFromDataManager) {}

TEST_F(DataManagerServiceTest, BEH_NodeUpSynchroniseFromDataManager) {}

TEST_F(DataManagerServiceTest, BEH_SetPmidOnlineFromPmidManager) {}

TEST_F(DataManagerServiceTest, BEH_SetPmidOfflineFromPmidManager) {}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
