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
#include "maidsafe/passport/types.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/parameters.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/mpid_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class MpidManagerServiceTest : public testing::Test {
 public:
  MpidManagerServiceTest()
      : pmid_(passport::CreatePmidAndSigner().first),
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_),
        routing_(pmid_),
        data_getter_(asio_service_, routing_),
        mpid_manager_service_(pmid_, routing_, vault_root_dir_, DiskUsage(10000)),
        asio_service_(2) {}

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

  DbMessageQueryResult Get(const ImmutableData::Name& data_name) const {
    return mpid_manager_service_.handler_.GetMessage(data_name);
  }

  void Put(const ImmutableData& data, const MpidName& mpid_name) {
    mpid_manager_service_.handler_.Put(data, mpid_name);
  }

 protected:
  virtual void SetUp() override {
    PmidName pmid_name(Identity(RandomString(64)));
    ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
    MpidManager::Key key(data.name());
    auto group_source(CreateGroupSource(data.name()));
  }

  passport::Pmid pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  MpidManagerService mpid_manager_service_;
  BoostAsioService asio_service_;
};

template <typename UnresolvedActionType>
void MpidManagerServiceTest::SendSync(
    const std::vector<UnresolvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresolvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void MpidManagerServiceTest::SendSync<MpidManager::UnresolvedPutAlert>(
     const std::vector<MpidManager::UnresolvedPutAlert>& unresolved_actions,
     const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MpidManagerService, MpidManager::UnresolvedPutAlert,
                                    SynchroniseFromMpidManagerToMpidManager>(
     &mpid_manager_service_, mpid_manager_service_.sync_put_alerts_, unresolved_actions,
     group_source);
}

template <>
void MpidManagerServiceTest::SendSync<MpidManager::UnresolvedDeleteAlert>(
    const std::vector<MpidManager::UnresolvedDeleteAlert>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MpidManagerService, MpidManager::UnresolvedDeleteAlert,
                                    SynchroniseFromMpidManagerToMpidManager>(
      &mpid_manager_service_, mpid_manager_service_.sync_delete_alerts_, unresolved_actions,
      group_source);
}

template <>
void MpidManagerServiceTest::SendSync<MpidManager::UnresolvedPutMessage>(
    const std::vector<MpidManager::UnresolvedPutMessage>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MpidManagerService, MpidManager::UnresolvedPutMessage,
                                    SynchroniseFromMpidManagerToMpidManager>(
      &mpid_manager_service_, mpid_manager_service_.sync_put_messages_, unresolved_actions,
      group_source);
}

template <>
void MpidManagerServiceTest::SendSync<MpidManager::UnresolvedDeleteMessage>(
    const std::vector<MpidManager::UnresolvedDeleteMessage>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MpidManagerService, MpidManager::UnresolvedDeleteMessage,
                                    SynchroniseFromMpidManagerToMpidManager>(
      &mpid_manager_service_, mpid_manager_service_.sync_delete_messages_, unresolved_actions,
      group_source);
}

template <>
std::vector<std::unique_ptr<MpidManager::UnresolvedPutAlert>>
MpidManagerServiceTest::GetUnresolvedActions<MpidManager::UnresolvedPutAlert>() {
  return mpid_manager_service_.sync_put_alerts_.GetUnresolvedActions();
}

template <>
std::vector<std::unique_ptr<MpidManager::UnresolvedDeleteAlert>>
MpidManagerServiceTest::GetUnresolvedActions<MpidManager::UnresolvedDeleteAlert>() {
  return mpid_manager_service_.sync_delete_alerts_.GetUnresolvedActions();
}

TEST_F(MpidManagerServiceTest, BEH_SendMessageRequestFromMpidNodeToMpidManager) {
  auto content(CreateContent<nfs::SendMessageRequestFromMpidNodeToMpidManager::Contents>());
  auto send_message_request(
           CreateMessage<nfs::SendMessageRequestFromMpidNodeToMpidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&mpid_manager_service_, send_message_request,
                                     routing::SingleSource(NodeId(content.base.sender->string())),
                                     routing::GroupId(NodeId(content.base.sender->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_DeleteRequestFromMpidNodeToMpidManager) {
  auto content(CreateContent<nfs::DeleteRequestFromMpidNodeToMpidManager::Contents>());
  auto send_message_request(
           CreateMessage<nfs::DeleteRequestFromMpidNodeToMpidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&mpid_manager_service_, send_message_request,
                                     routing::SingleSource(NodeId(content.base.sender->string())),
                                     routing::GroupId(NodeId(content.base.sender->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_GetMessageRequestFromMpidNodeToMpidManager) {
  using MessageType = nfs::GetMessageRequestFromMpidNodeToMpidManager;
  auto content(CreateContent<MessageType::Contents>());
  auto send_message_request(CreateMessage<MessageType>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&mpid_manager_service_, send_message_request,
                                     routing::SingleSource(NodeId(content.base.sender->string())),
                                     routing::GroupId(NodeId(content.base.sender->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_SendAlertFromMpidManagerToMpidManager) {
  using MessageType = SendAlertFromMpidManagerToMpidManager;
  auto content(CreateContent<MessageType::Contents>());
  auto send_alert_request(CreateMessage<MessageType>(content));
  auto group_source(CreateGroupSource(NodeId(content.base.sender->string())));
  EXPECT_NO_THROW(GroupSendToGroup(&mpid_manager_service_, send_alert_request, group_source,
                                   routing::GroupId(NodeId(content.base.receiver->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_GetMessageRequestFromMpidManagerToMpidManager) {
  using MessageType = GetRequestFromMpidManagerToMpidManager;
  auto content(CreateContent<MessageType::Contents>());
  auto get_message_request(CreateMessage<MessageType>(content));
  auto group_source(CreateGroupSource(NodeId(content.base.receiver->string())));
  EXPECT_NO_THROW(GroupSendToGroup(&mpid_manager_service_, get_message_request, group_source,
                                   routing::GroupId(NodeId(content.base.sender->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_DeleteRequestFromMpidManagerToMpidManager) {
  using MessageType = GetRequestFromMpidManagerToMpidManager;
  auto content(CreateContent<MessageType::Contents>());
  auto get_message_request(CreateMessage<MessageType>(content));
  auto group_source(CreateGroupSource(NodeId(content.base.receiver->string())));
  EXPECT_NO_THROW(GroupSendToGroup(&mpid_manager_service_, get_message_request, group_source,
                                   routing::GroupId(NodeId(content.base.sender->string()))));
}

TEST_F(MpidManagerServiceTest, BEH_SyncPutMessage) {
  passport::PublicMpid mpid(passport::CreateMpidAndSigner().first);
  auto message(CreateContent<nfs_vault::MpidMessage>());
  ImmutableData data(NonEmptyString(message.Serialise()));
  auto group_source(CreateGroupSource(mpid.name()));
  ActionMpidManagerPutMessage action_put(message, nfs::MessageId(RandomInt32()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MpidManager::UnresolvedPutMessage>(
          MpidManager::SyncGroupKey(mpid.name()), action_put, group_source));
  SendSync<MpidManager::UnresolvedPutMessage>(group_unresolved_action, group_source);
  EXPECT_TRUE(Get(data.name()).valid());
}

TEST_F(MpidManagerServiceTest, BEH_SyncDeleteMessage) {
  passport::PublicMpid mpid(passport::CreateMpidAndSigner().first);
  auto message(CreateContent<nfs_vault::MpidMessage>());
  ImmutableData data(NonEmptyString(message.Serialise()));
  Put(data, mpid.name());
  EXPECT_TRUE(Get(data.name()).valid());
  auto group_source(CreateGroupSource(mpid.name()));
  auto delete_message_action(ActionMpidManagerDeleteMessage(
                                 nfs_vault::MpidMessageAlert(
                                     message.base,
                                     nfs_vault::MessageIdType(data.name().value.string()))));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MpidManager::UnresolvedDeleteMessage>(
          MpidManager::SyncGroupKey(mpid.name()), delete_message_action, group_source));
  SendSync<MpidManager::UnresolvedDeleteMessage>(group_unresolved_action, group_source);
  EXPECT_FALSE(Get(data.name()).valid());
}

TEST_F(MpidManagerServiceTest, BEH_SyncPutAlert) {
  passport::PublicMpid mpid(passport::CreateMpidAndSigner().first);
  auto message(CreateContent<nfs_vault::MpidMessage>());
  ImmutableData data(NonEmptyString(message.Serialise()));
  auto alert(nfs_vault::MpidMessageAlert(message.base,
                                         nfs_vault::MessageIdType(data.name().value.string())));
  ImmutableData alert_data(NonEmptyString(alert.Serialise()));
  auto group_source(CreateGroupSource(mpid.name()));
  ActionMpidManagerPutAlert action_put(alert);
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MpidManager::UnresolvedPutAlert>(
          MpidManager::SyncGroupKey(mpid.name()), action_put, group_source));
  SendSync<MpidManager::UnresolvedPutAlert>(group_unresolved_action, group_source);
  EXPECT_TRUE(Get(alert_data.name()).valid());
}

TEST_F(MpidManagerServiceTest, BEH_SyncDeleteAlert) {
  passport::PublicMpid mpid(passport::CreateMpidAndSigner().first);
  auto message(CreateContent<nfs_vault::MpidMessage>());
  ImmutableData data(NonEmptyString(message.Serialise()));
  auto alert(nfs_vault::MpidMessageAlert(message.base,
                                         nfs_vault::MessageIdType(data.name().value.string())));
  ImmutableData alert_data(NonEmptyString(alert.Serialise()));
  Put(alert_data, mpid.name());
  EXPECT_TRUE(Get(alert_data.name()).valid());
  auto group_source(CreateGroupSource(mpid.name()));
  auto delete_alert_action(
           ActionMpidManagerDeleteAlert(
               nfs_vault::MpidMessageAlert(message.base,
                                           nfs_vault::MessageIdType(data.name().value.string()))));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MpidManager::UnresolvedDeleteAlert>(
               MpidManager::SyncGroupKey(mpid.name()), delete_alert_action, group_source));
  SendSync<MpidManager::UnresolvedDeleteAlert>(group_unresolved_action, group_source);
  EXPECT_FALSE(Get(alert_data.name()).valid());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
