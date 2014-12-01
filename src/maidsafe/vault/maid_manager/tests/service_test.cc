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

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

namespace test {

class MaidManagerServiceTest : public testing::Test {
 public:
  MaidManagerServiceTest()
      : anmaid_(),
        maid_(anmaid_),
        anpmid_(),
        pmid_(anpmid_),
        public_maid_(maid_),
        public_pmid_(pmid_),
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_),
        routing_(pmid_),
        data_getter_(asio_service_, routing_),
        maid_manager_service_(pmid_, routing_, data_getter_, vault_root_dir_),
        asio_service_(2) {}

  NodeId MaidNodeId() { return NodeId(maid_.name()->string()); }

  MaidManager::Value GetValue(const MaidManager::GroupName& group_name) {
    std::lock_guard<std::mutex> lock(maid_manager_service_.mutex_);
    return maid_manager_service_.accounts_.at(group_name);
  }

  void AddAccount(const MaidManager::GroupName& group_name,
                  const MaidManager::Value& value) {
    std::lock_guard<std::mutex> lock(maid_manager_service_.mutex_);
    maid_manager_service_.accounts_.insert(std::make_pair(group_name, value));
  }

  void CreateAccount() {
    MaidManager::Key key(public_maid_.name());
    MaidManager::Value value(100, 1000);
    AddAccount(public_maid_.name(), value);
  }

  template <typename ActionType>
  void Commit(const MaidManager::Key& key, const ActionType& action) {
    auto it(maid_manager_service_.accounts_.find(key));
    assert(it != std::end(maid_manager_service_.accounts_));
    action(it->second);
  }

  MaidManager::Value Get(const MaidManager::Key& key) {
    return maid_manager_service_.accounts_.find(key)->second;
  }

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

  bool Equal(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
    if (lhs.data_stored != rhs.data_stored || lhs.space_available != rhs.space_available)
      return false;
    return true;
  }

 protected:
  passport::Anmaid anmaid_;
  passport::Maid maid_;
  passport::Anpmid anpmid_;
  passport::Pmid pmid_;
  passport::PublicMaid public_maid_;
  passport::PublicPmid public_pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  MaidManagerService maid_manager_service_;
  AsioService asio_service_;
};

template <typename UnresolvedActionType>
void MaidManagerServiceTest::SendSync(
    const std::vector<UnresolvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedCreateAccount>(
    const std::vector<MaidManager::UnresolvedCreateAccount>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedCreateAccount,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_create_accounts_, unresolved_actions,
      group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedRemoveAccount>(
    const std::vector<MaidManager::UnresolvedRemoveAccount>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedRemoveAccount,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_remove_accounts_, unresolved_actions,
      group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedPut>(
    const std::vector<MaidManager::UnresolvedPut>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedPut,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_puts_, unresolved_actions, group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedDelete>(
    const std::vector<MaidManager::UnresolvedDelete>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedDelete,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_deletes_, unresolved_actions,
      group_source);
}

TEST_F(MaidManagerServiceTest, BEH_PutRequestFromMaidNodeToMaidManager) {
  CreateAccount();
  auto content(CreateContent<nfs::PutRequestFromMaidNodeToMaidManager::Contents>());
  auto put_request(CreateMessage<nfs::PutRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, put_request,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_PutResponseFromDataManagerToMaidManager) {
  auto content(CreateContent<PutResponseFromDataManagerToMaidManager::Contents>());
  NodeId data_name_id(content.name.raw_name.string());
  auto put_response(CreateMessage<PutResponseFromDataManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(data_name_id));
  EXPECT_NO_THROW(GroupSendToGroup(&maid_manager_service_, put_response, group_source,
                                   routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_PutFailureFromDataManagerToMaidManager) {
  auto content(CreateContent<PutFailureFromDataManagerToMaidManager::Contents>());
  NodeId data_name_id(content.name.raw_name.string()), maid_node_id(NodeId::IdType::kRandomId);
  auto put_failure(CreateMessage<PutFailureFromDataManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(data_name_id));
  EXPECT_NO_THROW(GroupSendToGroup(&maid_manager_service_, put_failure, group_source,
                                   routing::GroupId(maid_node_id)));
}

TEST_F(MaidManagerServiceTest, BEH_nfsDeleteRequestFromMaidNodeToMaidManager) {
  CreateAccount();
  auto content(CreateContent<nfs::DeleteRequestFromMaidNodeToMaidManager::Contents>());
  auto delete_request(CreateMessage<nfs::DeleteRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, delete_request,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsDeleteBranchUntilForkRequestFromMaidNodeToMaidManager) {
  auto content(
      CreateContent<nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Contents>());
  auto delete_branch(
      CreateMessage<nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, delete_branch,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsCreateAccountRequestFromMaidNodeToMaidManager) {
  nfs::CreateAccountRequestFromMaidNodeToMaidManager::Contents content(
      (passport::PublicMaid(maid_)), (passport::PublicAnmaid(anmaid_)));
  auto create_account(CreateMessage<nfs::CreateAccountRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, create_account,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsRemoveAccountRequestFromMaidNodeToMaidManager) {
  nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Contents content(anmaid_);
  auto remove_account(CreateMessage<nfs::RemoveAccountRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, remove_account,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_CreateAccount) {
  nfs::MessageId message_id(RandomInt32());
  ActionCreateAccount action_create_account(message_id);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedCreateAccount>(
      metadata_key, action_create_account, group_source));
  SendSync<MaidManager::UnresolvedCreateAccount>(group_unresolved_action, group_source);
  EXPECT_NO_THROW(GetValue(public_maid_.name()));
}

TEST_F(MaidManagerServiceTest, BEH_RemoveAccount) {
  CreateAccount();
  ActionRemoveAccount action_remove_account((nfs::MessageId(RandomUint32())));
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedRemoveAccount>(
      metadata_key, action_remove_account, group_source));
  SendSync<MaidManager::UnresolvedRemoveAccount>(group_unresolved_action, group_source);
  EXPECT_ANY_THROW(GetValue(public_maid_.name()));
}

TEST_F(MaidManagerServiceTest, BEH_Put) {
  CreateAccount();
  ActionMaidManagerPut action_put(kTestChunkSize);
  MaidManager::SyncKey key(
    public_maid_.name(), Identity(RandomString(64)), ImmutableData::Tag::kValue);
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MaidManager::UnresolvedPut>(key, action_put, group_source));
  SendSync<MaidManager::UnresolvedPut>(group_unresolved_action, group_source);
}

TEST_F(MaidManagerServiceTest, BEH_Delete) {
  CreateAccount();
  ActionMaidManagerDelete action_delete((nfs::MessageId(RandomInt32())));
  Identity data_name_id(RandomString(64));
  MaidManager::SyncKey key(public_maid_.name(), data_name_id, ImmutableData::Tag::kValue);
  Commit(key.group_name(), ActionMaidManagerPut(kTestChunkSize));
  EXPECT_NO_THROW(Get(key.group_name()));
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedDelete>(
      key, action_delete, group_source));
  SendSync<MaidManager::UnresolvedDelete>(group_unresolved_action, group_source);
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
