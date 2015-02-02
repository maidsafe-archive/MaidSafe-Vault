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
#include "maidsafe/routing/parameters.h"

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

  void CancelGetTimerTask(int task_id) {
    data_manager_service_.get_timer_.CancelTask(task_id);
  }

  void DeleteFromLruCache(const DataManager::Key& key) {
    data_manager_service_.lru_cache_.Delete(key);
  }

  template <typename Data>
  void GetForReplication(const PmidName& pmid_name, const typename Data::Name& data_name) {
    data_manager_service_.GetForReplication<Data>(pmid_name, data_name);
  }

  void AddToCloseNodesChange(const PmidName& pmid_name) {
    data_manager_service_.close_nodes_change_  =
        routing::CloseNodesChange(routing_.kNodeId(), std::vector<NodeId>(),
                                  std::vector<NodeId> { NodeId(pmid_name->string()) });
  }

  template <typename ActionType>
  void Commit(const DataManager::Key& key, const ActionType& action) {
    data_manager_service_.db_.Commit(key, action);
  }

  DataManager::Value Get(const DataManager::Key& key) { return data_manager_service_.db_.Get(key); }
  uint64_t Replicate(const DataManager::Key& key, nfs::MessageId message_id,
                     const PmidName& tried_pmid_node = PmidName()) {
    return data_manager_service_.Replicate(key, message_id, tried_pmid_node);
  }

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  virtual void SetUp() override {
    PmidName pmid_name(Identity(RandomString(64)));
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
  BoostAsioService asio_service_;
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
     &data_manager_service_, data_manager_service_.sync_puts_, unresolved_actions,
     group_source);
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

TEST_F(DataManagerServiceTest, BEH_PutRequestFromMaidManagerToDataManager) {
  NodeId maid_node_id(RandomString(NodeId::kSize)), data_name_id;
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.name.raw_name.string());
  auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_request, group_source,
                                   routing::GroupId(data_name_id)));
}

TEST_F(DataManagerServiceTest, BEH_PutResponseFromPmidManagerToDataManager) {
  NodeId data_name_id, pmid_node_id(RandomString(NodeId::kSize));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name().value, ImmutableData::Tag::kValue);
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  data_name_id = NodeId(key.name.string());
  auto content(CreateContent<PutResponseFromPmidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_response(CreateMessage<PutResponseFromPmidManagerToDataManager>(content));
  EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_response, group_source,
                                   routing::GroupId(data_name_id)));
  EXPECT_EQ(GetUnresolvedActions<DataManager::UnresolvedAddPmid>().size(), 0);
}

TEST_F(DataManagerServiceTest, BEH_PutFailureFromPmidManagerToDataManager) {
  NodeId data_name_id, pmid_node_id(RandomString(NodeId::kSize));
  auto content(CreateContent<PutFailureFromPmidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.name.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_failure(CreateMessage<PutFailureFromPmidManagerToDataManager>(content));
  EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_failure, group_source,
                                   routing::GroupId(data_name_id)));
  EXPECT_EQ(GetUnresolvedActions<DataManager::UnresolvedRemovePmid>().size(), 0);
}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromDataGetterToDataManager) {
  NodeId data_name_id, maid_node_id(RandomString(NodeId::kSize));
  auto content(CreateContent<nfs::GetRequestFromDataGetterToDataManager::Contents>());
  data_name_id = NodeId(content.raw_name.string());
  auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                     routing::SingleSource(maid_node_id),
                                     routing::GroupId(data_name_id)));
}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromDataGetterPartialToDataManager) {
  NodeId data_name_id, maid_node_id(RandomString(NodeId::kSize));
  auto content(CreateContent<nfs::GetRequestFromDataGetterPartialToDataManager::Contents>());
  data_name_id = NodeId(content.raw_name.string());
  auto get_request(CreateMessage<nfs::GetRequestFromDataGetterPartialToDataManager>(content));
  EXPECT_NO_THROW(SingleRelaySendsToGroup(
      &data_manager_service_, get_request,
      routing::SingleRelaySource(routing::SingleSource(maid_node_id), maid_node_id,
                                 routing::SingleSource(maid_node_id)),
      routing::GroupId(data_name_id)));
}

TEST_F(DataManagerServiceTest, BEH_GetResponseFromPmidNodeToDataManager) {
  NodeId pmid_node_id(RandomString(NodeId::kSize));
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

TEST_F(DataManagerServiceTest, BEH_DeleteRequestFromMaidManagerToDataManager) {
  NodeId maid_node_id(RandomString(NodeId::kSize));
  auto content(CreateContent<DeleteRequestFromMaidManagerToDataManager::Contents>());
  auto delete_request(CreateMessage<DeleteRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  GroupSendToGroup(&data_manager_service_, delete_request, group_source,
                   routing::GroupId(NodeId(content.raw_name.string())));
  EXPECT_TRUE(GetUnresolvedActions<DataManager::UnresolvedDelete>().size() == 0);
}

TEST_F(DataManagerServiceTest, BEH_FirstPut) {
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  NodeId maid_node_id(RandomString(NodeId::kSize)), data_name_id;
  data_name_id = NodeId(content.name.raw_name.string());
  auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_request, group_source,
                                   routing::GroupId(data_name_id)));
  // The entry shall only be created when received PutResponse from PmidManager
  DataManager::Key key(content.name.raw_name, content.name.type);
  EXPECT_ANY_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_Put) {
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  PmidName pmid_name(Identity(RandomString(64)));
  DataManager::Key key(content.name.raw_name, content.name.type);
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomInt32())));
  Commit(key, ActionDataManagerAddPmid(pmid_name));
  NodeId maid_node_id(RandomString(NodeId::kSize)),
         data_name_id { NodeId(content.name.raw_name.string()) };
  auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  EXPECT_NO_THROW(GroupSendToGroup(&data_manager_service_, put_request, group_source,
                                   routing::GroupId(data_name_id)));
  EXPECT_EQ(Get(key).chunk_size(), kTestChunkSize);
}

TEST_F(DataManagerServiceTest, BEH_SyncPut) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  ActionDataManagerPut action_put(kTestChunkSize, nfs::MessageId(RandomInt32()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<DataManager::UnresolvedPut>(key, action_put, group_source));
  SendSync<DataManager::UnresolvedPut>(group_unresolved_action, group_source);
  EXPECT_NO_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_Delete) {
  PmidName pmid_name(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomInt32())));
  auto group_source(CreateGroupSource(data.name()));
  // store key value in db
  Commit(key, ActionDataManagerAddPmid(pmid_name));
  EXPECT_TRUE(Get(key).chunk_size() == kTestChunkSize);
  // key value is in db
  ActionDataManagerDelete action_delete((nfs::MessageId(RandomInt32())));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<DataManager::UnresolvedDelete>(key, action_delete, group_source));
  SendSync<DataManager::UnresolvedDelete>(group_unresolved_action, group_source);
  EXPECT_ANY_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_AddPmidNoAccount) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  {
    PmidName pmid_name(Identity(RandomString(64)));
    ActionDataManagerAddPmid action_add_pmid(pmid_name);
    auto group_unresolved_action(CreateGroupUnresolvedAction<
                                 DataManager::UnresolvedAddPmid>(key, action_add_pmid,
                                                                 group_source));
    EXPECT_NO_THROW(SendSync<DataManager::UnresolvedAddPmid>(group_unresolved_action,
                                                             group_source));
  }
}

TEST_F(DataManagerServiceTest, BEH_AddPmid) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  auto group_source(CreateGroupSource(data.name()));
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  {  // Add first Pmid
    PmidName pmid_name(Identity(RandomString(64)));
    ActionDataManagerAddPmid action_add_pmid(pmid_name);
    auto group_unresolved_action(CreateGroupUnresolvedAction<
        DataManager::UnresolvedAddPmid>(key, action_add_pmid, group_source));
    SendSync<DataManager::UnresolvedAddPmid>(group_unresolved_action, group_source);
    auto value(Get(key));
    EXPECT_EQ(value.AllPmids().size(), 1);
    EXPECT_EQ(value.chunk_size(), kTestChunkSize);
  }
  {  // Add second Pmid
    PmidName pmid_name(Identity(RandomString(64)));
    ActionDataManagerAddPmid action_add_pmid(pmid_name);
    auto group_unresolved_action(CreateGroupUnresolvedAction<
        DataManager::UnresolvedAddPmid>(key, action_add_pmid, group_source));
    SendSync<DataManager::UnresolvedAddPmid>(group_unresolved_action, group_source);
    auto value(Get(key));
    EXPECT_EQ(value.AllPmids().size(), 2);
    EXPECT_EQ(value.chunk_size(), kTestChunkSize);
  }
}

TEST_F(DataManagerServiceTest, BEH_RemovePmid) {
  PmidName pmid_name(Identity(RandomString(64)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  auto group_source(CreateGroupSource(data.name()));
  // store key value in db
  PmidName pmid_name_two(Identity(RandomString(64)));
  Commit(key, ActionDataManagerAddPmid(pmid_name));
  Commit(key, ActionDataManagerAddPmid(pmid_name_two));
  auto value(Get(key));
  EXPECT_TRUE(value.chunk_size() == kTestChunkSize);
  EXPECT_TRUE(value.AllPmids().size() == 2);

  // Sync remove pmid
  ActionDataManagerRemovePmid action_remove_pmid(pmid_name_two);
  auto group_unresolved_action(CreateGroupUnresolvedAction<DataManager::UnresolvedRemovePmid>(
      key, action_remove_pmid, group_source));
  SendSync<DataManager::UnresolvedRemovePmid>(group_unresolved_action, group_source);
  EXPECT_TRUE(Get(key).AllPmids().size() == 1);
}

TEST_F(DataManagerServiceTest, BEH_ReplicateWithNoEntry) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  EXPECT_EQ(Replicate(vault::Key(data.name()), nfs::MessageId(RandomUint32())), 0);
}

TEST_F(DataManagerServiceTest, BEH_ReplicateNoPmid) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomUint32())));
  EXPECT_EQ(Replicate(vault::Key(data.name()), nfs::MessageId(RandomUint32())), 0);
}

TEST_F(DataManagerServiceTest, BEH_AccountTransferFromDataManagerToDataManager) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  DataManagerValue value(kTestChunkSize);
  protobuf::AccountTransfer account_transfer_proto;
  protobuf::DataManagerKeyValuePair kv_msg;
  kv_msg.set_key(key.Serialise());
  kv_msg.set_value(value.Serialise());
  account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
  auto content(AccountTransferFromDataManagerToDataManager::Contents(
                   account_transfer_proto.SerializeAsString()));
  auto account_transfer(CreateMessage<AccountTransferFromDataManagerToDataManager>(content));
  for (unsigned int index(0); index < routing::Parameters::group_size - 1; ++index) {
    EXPECT_NO_THROW(SingleSendsToSingle(
        &data_manager_service_, account_transfer,
        routing::SingleSource(NodeId(RandomString(NodeId::kSize))),
        routing::SingleId(routing_.kNodeId())));
  }
  EXPECT_NO_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_AccountQueryResponseFromDataManagerToDataManager) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  DataManager::Key key(data.name());
  DataManagerValue value(kTestChunkSize);
  protobuf::AccountTransfer account_transfer_proto;
  protobuf::DataManagerKeyValuePair kv_msg;
  auto group_source(CreateGroupSource(NodeId(data.name()->string())));
  kv_msg.set_key(key.Serialise());
  kv_msg.set_value(value.Serialise());
  account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
  auto content(AccountQueryResponseFromDataManagerToDataManager::Contents(
                   account_transfer_proto.SerializeAsString()));
  auto account_transfer(CreateMessage<AccountQueryResponseFromDataManagerToDataManager>(content));
  EXPECT_NO_THROW(GroupSendToSingle(&data_manager_service_, account_transfer, group_source,
                                    routing::SingleId(routing_.kNodeId())));
  EXPECT_NO_THROW(Get(key));
}

TEST_F(DataManagerServiceTest, BEH_GetForReplication) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  PmidName pmid_name(Identity(RandomString(64)));
  DataManager::Key key(data.name());
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomInt32())));
  Commit(key, ActionDataManagerAddPmid(pmid_name));
  this->DeleteFromLruCache(key);
  this->AddToCloseNodesChange(pmid_name);
  NodeId maid_node_id(RandomString(NodeId::kSize)),
  data_name_id { NodeId(data.name()->string()) };
  this->GetForReplication<ImmutableData>(PmidName(Identity(RandomString(64))), data.name());
  EXPECT_EQ(Get(key).chunk_size(), kTestChunkSize);
  data_manager_service_.Stop();
}

TEST_F(DataManagerServiceTest, BEH_DoHandleGetResponse) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  auto data_name(data.name());
  PmidName pmid_name(Identity(RandomString(64)));
  DataManager::Key key(data_name);
  Commit(key, ActionDataManagerPut(kTestChunkSize, nfs::MessageId(RandomInt32())));
  Commit(key, ActionDataManagerAddPmid(pmid_name));
  this->DeleteFromLruCache(key);
  this->AddToCloseNodesChange(pmid_name);
  NodeId maid_node_id(RandomString(NodeId::kSize)),
  data_name_id { NodeId(data_name->string()) };
  auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(
                       nfs_vault::DataName(data_name)));
  EXPECT_NO_THROW(SingleSendsToGroup(&data_manager_service_, get_request,
                                     routing::SingleSource(maid_node_id),
                                     routing::GroupId(data_name_id)));
  nfs_vault::DataNameAndContentOrCheckResult response(data_name, data.Serialise());
  auto get_response(CreateMessage<GetResponseFromPmidNodeToDataManager>(response));
  get_response.id = get_request.id;
  EXPECT_NO_THROW(SingleSendsToSingle(&data_manager_service_, get_response,
                                      routing::SingleSource(NodeId(pmid_name->string())),
                                      routing::SingleId(data_name_id)));
  Sleep(std::chrono::seconds(1));
  EXPECT_EQ(Get(key).chunk_size(), kTestChunkSize);
  data_manager_service_.Stop();
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
