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
#include "maidsafe/passport/passport.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/routing/close_nodes_change.h"

#include "maidsafe/vault/account_transfer.pb.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/unresolved_action.pb.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class PmidManagerServiceTest : public testing::Test {
 public:
  PmidManagerServiceTest()
      : pmid_(passport::CreatePmidAndSigner().first),
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_),
        routing_(pmid_),
        pmid_manager_service_(pmid_, routing_) {}

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  void AddGroup(PmidManager::Key key, const PmidManager::Value& value) {
    pmid_manager_service_.accounts_.insert(std::make_pair(key, value));
  }

  PmidManager::Value GetValue(const PmidManager::Key& key) {
    return pmid_manager_service_.accounts_.at(key);
  }

  template <typename ActionType>
  void Commit(const PmidManager::Key& key, const ActionType& action) {
    auto& value_ref(pmid_manager_service_.accounts_.at(key));
    action(value_ref);
  }

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  passport::Pmid pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  PmidManagerService pmid_manager_service_;
};

template <typename UnresolvedActionType>
void PmidManagerServiceTest::SendSync(
    const std::vector<UnresolvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void PmidManagerServiceTest::SendSync<PmidManager::UnresolvedPut>(
    const std::vector<PmidManager::UnresolvedPut>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedPut,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_puts_, unresolved_actions, group_source);
}

template <>
void PmidManagerServiceTest::SendSync<PmidManager::UnresolvedDelete>(
    const std::vector<PmidManager::UnresolvedDelete>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedDelete,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_deletes_, unresolved_actions,
      group_source);
}

template <>
void PmidManagerServiceTest::SendSync<PmidManager::UnresolvedCreateAccount>(
    const std::vector<PmidManager::UnresolvedCreateAccount>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedCreateAccount,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_create_account_, unresolved_actions,
      group_source);
}

template <>
void PmidManagerServiceTest::SendSync<PmidManager::UnresolvedUpdateAccount>(
    const std::vector<PmidManager::UnresolvedUpdateAccount>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<PmidManagerService, PmidManager::UnresolvedUpdateAccount,
                                    SynchroniseFromPmidManagerToPmidManager>(
      &pmid_manager_service_, pmid_manager_service_.sync_update_account_, unresolved_actions,
      group_source);
}

template <typename UnresolvedActionType>
std::vector<std::unique_ptr<UnresolvedActionType>> PmidManagerServiceTest::GetUnresolvedActions() {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
  return std::vector<std::unique_ptr<UnresolvedActionType>>();
}

template <>
std::vector<std::unique_ptr<PmidManager::UnresolvedPut>>
PmidManagerServiceTest::GetUnresolvedActions<PmidManager::UnresolvedPut>() {
  return pmid_manager_service_.sync_puts_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<PmidManager::UnresolvedDelete>>
PmidManagerServiceTest::GetUnresolvedActions<PmidManager::UnresolvedDelete>() {
  return pmid_manager_service_.sync_deletes_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<PmidManager::UnresolvedCreateAccount>>
PmidManagerServiceTest::GetUnresolvedActions<PmidManager::UnresolvedCreateAccount>() {
  return pmid_manager_service_.sync_create_account_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<PmidManager::UnresolvedUpdateAccount>>
PmidManagerServiceTest::GetUnresolvedActions<PmidManager::UnresolvedUpdateAccount>() {
return pmid_manager_service_.sync_update_account_.GetUnresolvedActions();
}

TEST_F(PmidManagerServiceTest, BEH_VariousRequests) {
  { //  PutRequestFromDataManagerToPmidManager
    auto content(CreateContent<PutRequestFromDataManagerToPmidManager::Contents>());
    auto put_request(CreateMessage<PutRequestFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(NodeId(put_request.contents->name.raw_name.string())));
    EXPECT_NO_THROW(GroupSendToGroup(&pmid_manager_service_, put_request, group_source,
                                     routing::GroupId(this->routing_.kNodeId())));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedPut>().size() == 0);
  }
  { //  PutFailureFromPmidNodeToPmidManager
    AddGroup(PmidName(pmid_.name()), PmidManagerValue());
    auto content(CreateContent<PutFailureFromPmidNodeToPmidManager::Contents>());
    auto put_failure(CreateMessage<PutFailureFromPmidNodeToPmidManager>(content));
    EXPECT_NO_THROW(SingleSendsToGroup(&pmid_manager_service_, put_failure,
                                       routing::SingleSource(NodeId(RandomString(NodeId::kSize))),
                                       routing::GroupId(NodeId(pmid_.name()->string()))));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedPut>().size() == 0);
    auto value(GetValue(PmidName(pmid_.name())));
    EXPECT_TRUE(value.offered_space == 0);
  }
  { // DeleteRequestFromDataManagerToPmidManager
    auto content(CreateContent<DeleteRequestFromDataManagerToPmidManager::Contents>());
    auto delete_request(CreateMessage<DeleteRequestFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(NodeId(content.name.raw_name.string())));
    EXPECT_NO_THROW(GroupSendToGroup(&pmid_manager_service_, delete_request, group_source,
                                     routing::GroupId(NodeId(pmid_.name()->string()))));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedDelete>().size() == 0);
  }
  { // CreatePmidAccountRequestFromMaidManagerToPmidManager
    auto content(CreateContent<CreatePmidAccountRequestFromMaidManagerToPmidManager::Contents>());
    auto create_account_request(
            CreateMessage<CreatePmidAccountRequestFromMaidManagerToPmidManager>(content));
    passport::Anmaid anmaid;
    passport::Maid maid(anmaid);
    auto group_source(CreateGroupSource(NodeId(maid.name()->string())));
    EXPECT_NO_THROW(GroupSendToGroup(&pmid_manager_service_, create_account_request, group_source,
                                    routing::GroupId(NodeId(pmid_.name()->string()))));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedCreateAccount>().size() == 0);
  }
  { // UpdateAccountFromDataManagerToPmidManager
    auto content(CreateContent<UpdateAccountFromDataManagerToPmidManager::Contents>());
    auto create_account_request(CreateMessage<UpdateAccountFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(NodeId(RandomString(64))));
    EXPECT_NO_THROW(GroupSendToGroup(&pmid_manager_service_, create_account_request, group_source,
                                    routing::GroupId(NodeId(pmid_.name()->string()))));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedUpdateAccount>().size() == 0);
  }
  { //  IntegrityCheckRequestFromDataManagerToPmidManager
    auto content(CreateContent<IntegrityCheckRequestFromDataManagerToPmidManager::Contents>());
    auto integrity_check_request(
            CreateMessage<IntegrityCheckRequestFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(
                          NodeId(integrity_check_request.contents->name.raw_name.string())));
    EXPECT_NO_THROW(GroupSendToGroup(&pmid_manager_service_, integrity_check_request,
                                     group_source, routing::GroupId(this->routing_.kNodeId())));
    EXPECT_TRUE(GetUnresolvedActions<PmidManager::UnresolvedDelete>().size() == 0);
  }
}

TEST_F(PmidManagerServiceTest, BEH_Create_Update_PmidAccount) {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  auto group_source(CreateGroupSource(NodeId(maid.name()->string())));
  PmidManager::SyncGroupKey key(PmidManager::Key(pmid_.name()));
  EXPECT_ANY_THROW(GetValue(PmidManager::Key(key.group_name())));
  {  // CreatePmidAccount
    ActionCreatePmidAccount action_create_account;
    auto group_unresolved_action(
        CreateGroupUnresolvedAction<PmidManager::UnresolvedCreateAccount>(
            key, action_create_account, group_source));
    SendSync<PmidManager::UnresolvedCreateAccount>(group_unresolved_action, group_source);
    try {
      auto value(GetValue(PmidManager::Key(key.group_name())));
      EXPECT_EQ(0, value.stored_total_size);
    } catch (std::exception& e) {
      EXPECT_TRUE(false) << boost::diagnostic_information(e);
    }
  }
  {  // UpdatePmidAccount
    auto size_diff(RandomInt32() % kMaxChunkSize - RandomInt32() % kMaxChunkSize);
    ActionPmidManagerUpdateAccount action_update_account(size_diff);
    auto group_unresolved_action(
        CreateGroupUnresolvedAction<PmidManager::UnresolvedUpdateAccount>(
            key, action_update_account, group_source));
    SendSync<PmidManager::UnresolvedUpdateAccount>(group_unresolved_action, group_source);
    try {
      auto value(GetValue(PmidManager::Key(key.group_name())));
      EXPECT_EQ(0, value.stored_total_size);
      EXPECT_EQ(size_diff, value.lost_total_size);
    } catch (std::exception& e) {
      EXPECT_TRUE(false) << boost::diagnostic_information(e);
    }
  }
}

TEST_F(PmidManagerServiceTest, BEH_Update_PmidAccount) {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  auto group_source(CreateGroupSource(NodeId(maid.name()->string())));
  PmidManager::SyncGroupKey key(PmidManager::Key(pmid_.name()));
  int64_t size_diff(RandomInt32() % kMaxChunkSize);
  size_diff -= RandomInt32() % kMaxChunkSize;
  ActionPmidManagerUpdateAccount action_update_account(size_diff);
  auto unresolved_update_action(
      CreateGroupUnresolvedAction<PmidManager::UnresolvedUpdateAccount>(
          key, action_update_account, group_source));
  {  // Update non-exists account
    SendSync<PmidManager::UnresolvedUpdateAccount>(unresolved_update_action, group_source);
    EXPECT_ANY_THROW(GetValue(PmidManager::Key(key.group_name())));
  }
  {  // Lost After Put
    ActionPmidManagerPut action_put(kTestChunkSize, nfs::MessageId(RandomInt32()));
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    auto unresolved_put_action(CreateGroupUnresolvedAction<PmidManager::UnresolvedPut>(
        PmidManager::SyncKey(PmidManager::Key(key.group_name()),
                             data.name(), ImmutableData::Tag::kValue),
        action_put, group_source));
    SendSync<PmidManager::UnresolvedPut>(unresolved_put_action, group_source);
    auto value(GetValue(PmidManager::Key(key.group_name())));
    EXPECT_EQ(kTestChunkSize, value.stored_total_size);
    auto unresolved_update_action(CreateGroupUnresolvedAction<
             PmidManager::UnresolvedUpdateAccount>(key, action_update_account, group_source));
    SendSync<PmidManager::UnresolvedUpdateAccount>(unresolved_update_action, group_source);
    try {
      auto value(GetValue(PmidManager::Key(key.group_name())));
      EXPECT_EQ(kTestChunkSize - size_diff, value.stored_total_size);
      EXPECT_EQ(size_diff, value.lost_total_size);
    } catch (std::exception& e) {
      EXPECT_TRUE(false) << boost::diagnostic_information(e);
    }
  }
}

TEST_F(PmidManagerServiceTest, BEH_PutThenDelete) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  auto group_source(CreateGroupSource(NodeId(pmid_.name()->string())));
  PmidManager::Key key(pmid_.name());
  // PmidManager needs to handle the situation that put request to non-registered
//  AddGroup(PmidName(pmid_.name()), PmidManagerValue());

  {  // Put
    ActionPmidManagerPut action_put(kTestChunkSize, nfs::MessageId(RandomInt32()));
    auto group_unresolved_action(
        CreateGroupUnresolvedAction<PmidManager::UnresolvedPut>(
            PmidManager::SyncKey(key, data.name(), ImmutableData::Tag::kValue),
            action_put, group_source));
    SendSync<PmidManager::UnresolvedPut>(group_unresolved_action, group_source);
    auto value(GetValue(key));
    EXPECT_EQ(kTestChunkSize, value.stored_total_size);
  }
  {  //  Delete
    ActionPmidManagerDelete action_delete(kTestChunkSize, false, false);
    EXPECT_NO_THROW(GetValue(PmidName(pmid_.name()))) << "pmid name should be here";
    auto group_unresolved_action(CreateGroupUnresolvedAction<PmidManager::UnresolvedDelete>(
        PmidManager::SyncKey(key, data.name(), ImmutableData::Tag::kValue),
        action_delete, group_source));
    SendSync<PmidManager::UnresolvedDelete>(group_unresolved_action, group_source);
    EXPECT_ANY_THROW(GetValue(key)) << " this key shoud have been deleted";
    EXPECT_THROW(GetValue(PmidName(pmid_.name())), std::exception)
        << " Added one pmid and deleted it so account should be removed";
  }
}

TEST_F(PmidManagerServiceTest, BEH_AccountTransferFromPmidManagerToPmidManager) {
  PmidManager::Key pmid_name(passport::CreatePmidAndSigner().first.name());
  MetadataKey<PmidName> key(pmid_name);
  PmidManagerValue value(kTestChunkSize, 0 , kTestChunkSize);
  protobuf::AccountTransfer account_transfer_proto;
  protobuf::PmidManagerKeyValuePair kv_msg;
  kv_msg.set_key(key.Serialise());
  kv_msg.set_value(value.Serialise());
  account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
  auto content(AccountTransferFromPmidManagerToPmidManager::Contents(
                  account_transfer_proto.SerializeAsString()));
  auto account_transfer(CreateMessage<AccountTransferFromPmidManagerToPmidManager>(content));
  for (unsigned int index(0); index < routing::Parameters::group_size - 1; ++index) {
    EXPECT_NO_THROW(SingleSendsToSingle(&pmid_manager_service_, account_transfer,
                       routing::SingleSource(NodeId(RandomString(NodeId::kSize))),
                       routing::SingleId(routing_.kNodeId())));
  }
  auto result(GetValue(pmid_name));
  EXPECT_EQ(value, result);
}

TEST_F(PmidManagerServiceTest, BEH_AccountQueryFromPmidManagerToPmidManager) {
  PmidManager::Key pmid_name(passport::CreatePmidAndSigner().first.name());
  auto content(CreateContent<AccountQueryFromPmidManagerToPmidManager::Contents>());
  auto account_query(CreateMessage<AccountQueryFromPmidManagerToPmidManager>(content));
  // query a non-existed account
  EXPECT_NO_THROW(SingleSendsToGroup(&pmid_manager_service_, account_query,
                                     routing::SingleSource(NodeId(RandomString(NodeId::kSize))),
                                     routing::GroupId(NodeId(pmid_name->string()))));
  // query an existing account
  AddGroup(pmid_name, PmidManagerValue());
  EXPECT_NO_THROW(SingleSendsToGroup(&pmid_manager_service_, account_query,
                                     routing::SingleSource(NodeId(RandomString(NodeId::kSize))),
                                     routing::GroupId(NodeId(pmid_name->string()))));
}

TEST_F(PmidManagerServiceTest, BEH_AccountQueryResponseFromPmidManagerToPmidManager) {
  PmidManager::Key pmid_name(passport::CreatePmidAndSigner().first.name());
  MetadataKey<PmidName> key(pmid_name);
  PmidManagerValue value(kTestChunkSize, 0 , kTestChunkSize);
  protobuf::AccountTransfer account_transfer_proto;
  protobuf::PmidManagerKeyValuePair kv_msg;
  auto group_source(CreateGroupSource(NodeId(pmid_name->string())));
  kv_msg.set_key(key.Serialise());
  kv_msg.set_value(value.Serialise());
  account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
  auto content(AccountQueryResponseFromPmidManagerToPmidManager::Contents(
                  account_transfer_proto.SerializeAsString()));
  auto account_transfer(CreateMessage<AccountQueryResponseFromPmidManagerToPmidManager>(content));
  EXPECT_NO_THROW(GroupSendToSingle(&pmid_manager_service_, account_transfer, group_source,
                                    routing::SingleId(routing_.kNodeId())));
  auto result(GetValue(pmid_name));
  EXPECT_EQ(value, result);
}

TEST_F(PmidManagerServiceTest, BEH_HandleChurn) {
  std::vector<NodeId> old_close_nodes, new_close_nodes;
  PmidName new_pmid_name(passport::CreatePmidAndSigner().first.name());
  NodeId new_node(new_pmid_name->string());
  new_close_nodes.push_back(new_node);
  NodeId vault_id(pmid_.name()->string());
  std::shared_ptr<routing::CloseNodesChange> close_node_change_ptr(new
      routing::CloseNodesChange(vault_id, old_close_nodes, new_close_nodes));
  EXPECT_NO_THROW(pmid_manager_service_.HandleChurnEvent(close_node_change_ptr));
  AddGroup(new_pmid_name, PmidManagerValue());
  EXPECT_NO_THROW(pmid_manager_service_.HandleChurnEvent(close_node_change_ptr));
  PmidName new_account(passport::CreatePmidAndSigner().first.name());
  AddGroup(new_account, PmidManagerValue());
  EXPECT_NO_THROW(pmid_manager_service_.HandleChurnEvent(close_node_change_ptr));
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
