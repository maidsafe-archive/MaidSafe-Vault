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
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class DataManagerServiceTest : public testing::Test {
 public:
  DataManagerServiceTest()
      : pmid_(passport::CreatePmidAndSigner().first),
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_),
        routing_(pmid_),
        data_getter_(asio_service_, routing_),
        data_manager_service_(pmid_, routing_, data_getter_, vault_root_dir_),
        asio_service_(2) {}

  typedef std::function<
      void(const std::pair<PmidName, GetResponseFromPmidNodeToDataManager::Contents>&)> Functor;

  void AddTask(Functor functor, uint32_t required, uint32_t task_id) {
    data_manager_service_.get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, required,
                                             task_id);
  }

  template <typename ActionType>
  void Commit(const DataManager::Key& key, const ActionType& action) {
    data_manager_service_.db_.Commit(key, action);
  }

  DataManager::Value Get(const DataManager::Key& key) { return data_manager_service_.db_.Get(key); }

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  virtual void SetUp() override {
    PmidName pmid_name(Identity(RandomString(64)));
    ActionDataManagerPut action_put;
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    DataManager::Key key(data.name());
    auto group_source(CreateGroupSource(data.name()));
  }
  passport::Pmid pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  DataManagerService data_manager_service_;
  AsioService asio_service_;
};

template <typename UnresolvedActionType>
void DataManagerServiceTest::SendSync(
    const std::vector<UnresolvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedPut>(
    const std::vector<DataManager::UnresolvedPut>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedPut,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_puts_, unresolved_actions, group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedDelete>(
    const std::vector<DataManager::UnresolvedDelete>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedDelete,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_deletes_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedAddPmid>(
    const std::vector<DataManager::UnresolvedAddPmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedAddPmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_add_pmids_, unresolved_actions,
      group_source);
}

template <>
void DataManagerServiceTest::SendSync<DataManager::UnresolvedRemovePmid>(
    const std::vector<DataManager::UnresolvedRemovePmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<DataManagerService, DataManager::UnresolvedRemovePmid,
                                    SynchroniseFromDataManagerToDataManager>(
      &data_manager_service_, data_manager_service_.sync_remove_pmids_, unresolved_actions,
      group_source);
}

template <typename UnresolvedActionType>
std::vector<std::unique_ptr<UnresolvedActionType>> DataManagerServiceTest::GetUnresolvedActions() {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
  return std::vector<std::unique_ptr<UnresolvedActionType>>();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedPut>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedPut>() {
  return data_manager_service_.sync_puts_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedDelete>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedDelete>() {
  return data_manager_service_.sync_deletes_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedAddPmid>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedAddPmid>() {
  return data_manager_service_.sync_add_pmids_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<DataManager::UnresolvedRemovePmid>>
DataManagerServiceTest::GetUnresolvedActions<DataManager::UnresolvedRemovePmid>() {
  return data_manager_service_.sync_remove_pmids_.GetUnresolvedActions();
}

TEST_F(DataManagerServiceTest, BEH_Varios) {
  //  PutRequestFromMaidManagerToDataManager
  {
    NodeId maid_node_id(NodeId::IdType::kRandomId), data_name_id;
    auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.data.name.raw_name.string());
    auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
    auto group_source(CreateGroupSource(maid_node_id));
    EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_request, group_source,
                                     routing::GroupId(data_name_id)));
    EXPECT_TRUE(GetUnresolvedActions<DataManager::UnresolvedPut>().size() == 0);
  }
  //  BEH_PutResponseFromPmidManagerToDataManager
  {
    NodeId data_name_id, pmid_node_id(NodeId::IdType::kRandomId);
    auto content(CreateContent<PutResponseFromPmidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.name.raw_name.string());
    auto group_source(CreateGroupSource(pmid_node_id));
    auto put_response(CreateMessage<PutResponseFromPmidManagerToDataManager>(content));
    EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_response, group_source,
                                     routing::GroupId(data_name_id)));
    EXPECT_EQ(GetUnresolvedActions<DataManager::UnresolvedAddPmid>().size(), 0);
  }
  //  PutFailureFromPmidManagerToDataManager
  {
    NodeId data_name_id, pmid_node_id(NodeId::IdType::kRandomId);
    auto content(CreateContent<PutFailureFromPmidManagerToDataManager::Contents>());
    data_name_id = NodeId(content.name.raw_name.string());
    auto group_source(CreateGroupSource(pmid_node_id));
    auto put_failure(CreateMessage<PutFailureFromPmidManagerToDataManager>(content));
    EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_failure, group_source,
                                     routing::GroupId(data_name_id)));
    EXPECT_EQ(GetUnresolvedActions<DataManager::UnresolvedRemovePmid>().size(), 0);
  }
  //  GetRequestFromMaidNodeToDataManager
  {
    NodeId data_name_id, maid_node_id(NodeId::IdType::kRandomId);
    auto content(CreateContent<nfs::GetRequestFromMaidNodeToDataManager::Contents>());
    data_name_id = NodeId(content.raw_name.string());
    auto get_request(CreateMessage<nfs::GetRequestFromMaidNodeToDataManager>(content));
    EXPECT_NO_THROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                       routing::SingleSource(maid_node_id),
                                       routing::GroupId(data_name_id)));
  }
  //  GetRequestFromDataGetterToDataManager
  {
    NodeId data_name_id, maid_node_id(NodeId::IdType::kRandomId);
    auto content(CreateContent<nfs::GetRequestFromDataGetterToDataManager::Contents>());
    data_name_id = NodeId(content.raw_name.string());
    auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(content));
    EXPECT_NO_THROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                       routing::SingleSource(maid_node_id),
                                       routing::GroupId(data_name_id)));
  }
  //  GetResponseFromPmidNodeToDataManager
  {
    NodeId pmid_node_id(NodeId::IdType::kRandomId);
    auto content(CreateContent<GetResponseFromPmidNodeToDataManager::Contents>());
    auto get_response(CreateMessage<GetResponseFromPmidNodeToDataManager>(content));
    auto functor([=](const std::pair<PmidName, GetResponseFromPmidNodeToDataManager::Contents>&) {
      LOG(kVerbose) << "functor called";
    });

    AddTask(functor, 1, get_response.id.data);
    EXPECT_NO_THROW(SingleSendsToSingle(&data_manager_service_, get_response,
                                        routing::SingleSource(pmid_node_id),
                                        routing::SingleId(routing_.kNodeId())));
  }
}

TEST_F(DataManagerServiceTest, BEH_GetCachedResponseFromCacheHandlerToDataManager) {
  NodeId cache_handler_id(NodeId::IdType::kRandomId);
  auto content(CreateContent<GetCachedResponseFromCacheHandlerToDataManager::Contents>());
  auto get_cache_response(CreateMessage<GetCachedResponseFromCacheHandlerToDataManager>(content));
  EXPECT_NO_THROW(SingleSendsToSingle(&data_manager_service_, get_cache_response,
                                      routing::SingleSource(cache_handler_id),
                                      routing::SingleId(routing_.kNodeId())));
}

TEST_F(DataManagerServiceTest, BEH_DeleteRequestFromMaidManagerToDataManager) {
  NodeId maid_node_id(NodeId::IdType::kRandomId);
  auto content(CreateContent<DeleteRequestFromMaidManagerToDataManager::Contents>());
  auto delete_request(CreateMessage<DeleteRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  GroupSendToGroup(&data_manager_service_, delete_request, group_source,
                   routing::GroupId(NodeId(content.raw_name.string())));
  EXPECT_TRUE(GetUnresolvedActions<DataManager::UnresolvedDelete>().size() == 0);
}


TEST_F(DataManagerServiceTest, BEH_Put) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<DataManager::UnresolvedPut>(key, action_put, group_source));
  SendSync<DataManager::UnresolvedPut>(group_unresolved_action, group_source);
}

TEST_F(DataManagerServiceTest, BEH_Delete) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  // store key value in db
  Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
  // key value is in db
  ActionDataManagerDelete action_delete((nfs::MessageId(RandomInt32())));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<DataManager::UnresolvedDelete>(key, action_delete, group_source));
  SendSync<DataManager::UnresolvedDelete>(group_unresolved_action, group_source);
  EXPECT_ANY_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_AddPmid) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  // check key value is not in db
  EXPECT_ANY_THROW(Get(key));
  // Sync AddPmid
  ActionDataManagerAddPmid action_add_pmid(pmid_name, kTestChunkSize);
  auto group_unresolved_action(CreateGroupUnresolvedAction<DataManager::UnresolvedAddPmid>(
      key, action_add_pmid, group_source));
  SendSync<DataManager::UnresolvedAddPmid>(group_unresolved_action, group_source);
}

TEST_F(DataManagerServiceTest, BEH_RemovePmid) {
  PmidName pmid_name(Identity(RandomString(64)));
  ActionDataManagerPut action_put;
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  // store key value in db
  PmidName pmid_name_two(Identity(RandomString(64)));
  Commit(key, ActionDataManagerAddPmid(pmid_name, kTestChunkSize));
  Commit(key, ActionDataManagerAddPmid(pmid_name_two, kTestChunkSize));
  auto value(Get(key));
  EXPECT_TRUE(value.AllPmids().size() == 2);

  // Sync remove pmid
  ActionDataManagerRemovePmid action_remove_pmid(pmid_name_two);
  auto group_unresolved_action(CreateGroupUnresolvedAction<DataManager::UnresolvedRemovePmid>(
      key, action_remove_pmid, group_source));
  SendSync<DataManager::UnresolvedRemovePmid>(group_unresolved_action, group_source);
  EXPECT_TRUE(Get(key).AllPmids().size() == 1);
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
