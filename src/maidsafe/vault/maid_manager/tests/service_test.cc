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
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/maid_manager/action_reference_count.h"

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
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_),
        routing_(pmid_),
        data_getter_(asio_service_, routing_),
        maid_manager_service_(pmid_, routing_, data_getter_, vault_root_dir_),
        asio_service_(2) {}

  NodeId MaidNodeId() { return NodeId(maid_.name()->string()); }

  MaidManager::Metadata GetMetadata(const MaidManager::GroupName& group_name) {
    return maid_manager_service_.group_db_.GetMetadata(group_name);
  }

  void AddMetadata(const MaidManager::GroupName& group_name,
                   const MaidManager::Metadata& metadata) {
    maid_manager_service_.group_db_.AddGroup(group_name, metadata);
  }

  void CreateAccount() {
    MaidManager::MetadataKey metadata_key(public_maid_.name());
    MaidManager::Metadata metadata(100, std::vector<PmidTotals>());
    AddMetadata(public_maid_.name(), metadata);
  }

  template <typename ActionType>
  void Commit(const MaidManager::Key& key, const ActionType& action) {
    maid_manager_service_.group_db_.Commit(key, action);
  }

  MaidManager::Value Get(const MaidManager::Key& key) {
    return maid_manager_service_.group_db_.GetValue(key);
  }

  std::vector<PmidTotals> MetadataPmidTotals(const MaidManager::Metadata& metadata) {
    return metadata.pmid_totals_;
  }

  void RegisterPmid() {
    nfs_vault::PmidRegistration pmid_registration(maid_, pmid_, false);
    maid_manager_service_.group_db_.Commit(
        public_maid_.name(),
        ActionMaidManagerRegisterPmid(pmid_registration, nfs::MessageId(RandomInt32())));
  }

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

  bool Equal(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs) {
    if (lhs.total_put_data_ != rhs.total_put_data_)
      return false;

    if (lhs.pmid_totals_.size() != rhs.pmid_totals_.size())
      return false;

    for (const auto& pmid_total : lhs.pmid_totals_) {
      auto found(std::find_if(
          std::begin(rhs.pmid_totals_), std::end(rhs.pmid_totals_),
          [&](const PmidTotals& rhs_pmid_total) { return pmid_total == rhs_pmid_total; }));
      if (found == std::end(rhs.pmid_totals_))
        return false;
    }
    return true;
  }

 protected:
  passport::Anmaid anmaid_;
  passport::Maid maid_;
  passport::Anpmid anpmid_;
  passport::Pmid pmid_;
  passport::PublicMaid public_maid_;
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

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedRegisterPmid>(
    const std::vector<MaidManager::UnresolvedRegisterPmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedRegisterPmid,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_register_pmids_, unresolved_actions,
      group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedUnregisterPmid>(
    const std::vector<MaidManager::UnresolvedUnregisterPmid>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedUnregisterPmid,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_unregister_pmids_, unresolved_actions,
      group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedUpdatePmidHealth>(
    const std::vector<MaidManager::UnresolvedUpdatePmidHealth>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedUpdatePmidHealth,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_update_pmid_healths_, unresolved_actions,
      group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedIncrementReferenceCounts>(
    const std::vector<MaidManager::UnresolvedIncrementReferenceCounts>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService,
                                    MaidManager::UnresolvedIncrementReferenceCounts,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_increment_reference_counts_,
      unresolved_actions, group_source);
}

template <>
void MaidManagerServiceTest::SendSync<MaidManager::UnresolvedDecrementReferenceCounts>(
    const std::vector<MaidManager::UnresolvedDecrementReferenceCounts>& unresolved_actions,
    const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService,
                                    MaidManager::UnresolvedDecrementReferenceCounts,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_decrement_reference_counts_,
      unresolved_actions, group_source);
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

TEST_F(MaidManagerServiceTest, BEH_nfsRegisterPmidRequestFromMaidNodeToMaidManager) {
  nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Contents content(maid_, pmid_, true);
  auto register_pmid(CreateMessage<nfs::RegisterPmidRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, register_pmid,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsUnregisterPmidRequestFromMaidNodeToMaidManager) {
  nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Contents content(
      nfs_vault::DataName(DataTagValue::kPmidValue, Identity(pmid_.name()->string())));
  auto unregister_pmid(CreateMessage<nfs::UnregisterPmidRequestFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, unregister_pmid,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_PmidHealthResponseFromPmidManagerToMaidManager) {
  PmidName pmid_name(pmid_.name());
  PmidManagerMetadata pmid_manager_metadata(pmid_name);
  pmid_manager_metadata.SetAvailableSize(kTestChunkSize * (RandomUint32() % kAverageChunksStored));
  PmidHealthResponseFromPmidManagerToMaidManager::Contents content(
      nfs_vault::PmidHealth(pmid_manager_metadata.Serialise()),
      nfs_client::ReturnCode(CommonErrors::success));
  auto pmid_health_response(CreateMessage<PmidHealthResponseFromPmidManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(NodeId(pmid_name->string())));
  EXPECT_NO_THROW(GroupSendToGroup(&maid_manager_service_, pmid_health_response, group_source,
                                   routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsIncrementReferenceCountsFromMaidNodeToMaidManager) {
  nfs::IncrementReferenceCountsFromMaidNodeToMaidManager::Contents content;
  auto increment_reference(
      CreateMessage<nfs::IncrementReferenceCountsFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, increment_reference,
                                     routing::SingleSource(MaidNodeId()),
                                     routing::GroupId(MaidNodeId())));
}

TEST_F(MaidManagerServiceTest, BEH_nfsDecrementReferenceCountsFromMaidNodeToMaidManager) {
  nfs::DecrementReferenceCountsFromMaidNodeToMaidManager::Contents content;
  auto decrement_reference(
      CreateMessage<nfs::DecrementReferenceCountsFromMaidNodeToMaidManager>(content));
  EXPECT_NO_THROW(SingleSendsToGroup(&maid_manager_service_, decrement_reference,
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
  EXPECT_NO_THROW(GetMetadata(public_maid_.name()));
}

TEST_F(MaidManagerServiceTest, BEH_RemoveAccount) {
  CreateAccount();
  ActionRemoveAccount action_remove_account((nfs::MessageId(RandomUint32())));
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedRemoveAccount>(
      metadata_key, action_remove_account, group_source));
  SendSync<MaidManager::UnresolvedRemoveAccount>(group_unresolved_action, group_source);
  EXPECT_ANY_THROW(GetMetadata(public_maid_.name()));
}

TEST_F(MaidManagerServiceTest, BEH_Put) {
  CreateAccount();
  ActionMaidManagerPut action_put(kTestChunkSize);
  MaidManager::Key key(public_maid_.name(), Identity(RandomString(64)), ImmutableData::Tag::kValue);
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MaidManager::UnresolvedPut>(key, action_put, group_source));
  SendSync<MaidManager::UnresolvedPut>(group_unresolved_action, group_source);
}

TEST_F(MaidManagerServiceTest, BEH_Delete) {
  CreateAccount();
  ActionMaidManagerDelete action_delete((nfs::MessageId(RandomInt32())));
  Identity data_name_id(RandomString(64));
  MaidManager::Key key(public_maid_.name(), data_name_id, ImmutableData::Tag::kValue);
  Commit(key, ActionMaidManagerPut(kTestChunkSize));
  EXPECT_NO_THROW(Get(key));
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MaidManager::UnresolvedDelete>(key, action_delete, group_source));
  SendSync<MaidManager::UnresolvedDelete>(group_unresolved_action, group_source);
}

TEST_F(MaidManagerServiceTest, BEH_RegistedPmid) {
  CreateAccount();
  nfs_vault::PmidRegistration pmid_registration(maid_, pmid_, false);
  ActionMaidManagerRegisterPmid action_register_pmid(pmid_registration,
                                                     nfs::MessageId(RandomUint32()));
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedRegisterPmid>(
      metadata_key, action_register_pmid, group_source));
  SendSync<MaidManager::UnresolvedRegisterPmid>(group_unresolved_action, group_source);
  MaidManager::Metadata metadata(GetMetadata(public_maid_.name()));
  EXPECT_TRUE(MetadataPmidTotals(metadata).size() ==
              1);  // FAILS BECAUSE DATAGETTER GET NEVER SUCCEEDS
}

TEST_F(MaidManagerServiceTest, BEH_UnregistedPmid) {
  CreateAccount();
  RegisterPmid();
  MaidManager::Metadata metadata(GetMetadata(public_maid_.name()));
  EXPECT_TRUE(MetadataPmidTotals(metadata).size() == 1);
  ActionMaidManagerUnregisterPmid action_unregister_pmid(
      PmidName(Identity(pmid_.name()->string())));
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedUnregisterPmid>(
      metadata_key, action_unregister_pmid, group_source));
  SendSync<MaidManager::UnresolvedUnregisterPmid>(group_unresolved_action, group_source);
  metadata = GetMetadata(public_maid_.name());
  EXPECT_TRUE(MetadataPmidTotals(metadata).empty());
}

TEST_F(MaidManagerServiceTest, BEH_UpdatePmid) {
  PmidManagerMetadata pmid_manager_metadata(PmidName(Identity(pmid_.name()->string())));
  pmid_manager_metadata.stored_count = 1;
  pmid_manager_metadata.stored_total_size = kTestChunkSize;
  pmid_manager_metadata.lost_count = 2;
  pmid_manager_metadata.lost_total_size = 2 * kTestChunkSize;
  pmid_manager_metadata.claimed_available_size = 2 << 20;
  PmidTotals pmid_totals(std::string(), pmid_manager_metadata);
  MaidManager::Metadata metadata(100, std::vector<PmidTotals>(1, pmid_totals));
  AddMetadata(public_maid_.name(), metadata);
  pmid_manager_metadata.stored_count = 4;
  pmid_manager_metadata.stored_total_size = kTestChunkSize * 4;
  pmid_manager_metadata.lost_count = 6;
  pmid_manager_metadata.lost_total_size = 6 * kTestChunkSize;
  pmid_manager_metadata.claimed_available_size = 2 << 10;
  PmidTotals updated_pmid_totals(std::string(), pmid_manager_metadata);
  MaidManager::Metadata updated_metadata(100, std::vector<PmidTotals>(1, updated_pmid_totals));
  ActionMaidManagerUpdatePmidHealth action_update_pmid_health(pmid_manager_metadata);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(CreateGroupUnresolvedAction<MaidManager::UnresolvedUpdatePmidHealth>(
      metadata_key, action_update_pmid_health, group_source));
  SendSync<MaidManager::UnresolvedUpdatePmidHealth>(group_unresolved_action, group_source);
  MaidManager::Metadata stored_metadata(GetMetadata(public_maid_.name()));
  EXPECT_TRUE(Equal(stored_metadata, updated_metadata));
}

TEST_F(MaidManagerServiceTest, BEH_IncrementReferenceCounts) {
  const size_t kChunksCount(10);
  std::vector<ImmutableData::Name> chunks_list;
  std::vector<MaidManager::Key> keys_list;
  CreateAccount();
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  for (size_t index(0); index < kChunksCount; ++index) {
    Identity data_name_id(RandomString(64));
    MaidManager::Key key(public_maid_.name(), data_name_id, ImmutableData::Tag::kValue);
    Commit(key, ActionMaidManagerPut(kTestChunkSize));
    EXPECT_NO_THROW(Get(key));
    chunks_list.push_back(ImmutableData::Name(data_name_id));
    keys_list.push_back(key);
  }
  ActionMaidManagerIncrementReferenceCounts action_increment_reference(
      (nfs_vault::DataNames(chunks_list)));
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MaidManager::UnresolvedIncrementReferenceCounts>(
          metadata_key, action_increment_reference, group_source));
  SendSync<MaidManager::UnresolvedIncrementReferenceCounts>(group_unresolved_action, group_source);
  for (size_t index(0); index < kChunksCount; ++index) {
    auto value(Get(keys_list[index]));
    EXPECT_TRUE(value.count() == 2);
  }
}

TEST_F(MaidManagerServiceTest, BEH_DecrementReferenceCounts) {
  const size_t kChunksCount(10);
  std::vector<ImmutableData::Name> chunks_list;
  std::vector<MaidManager::Key> keys_list;
  CreateAccount();
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  for (size_t index(0); index < kChunksCount; ++index) {
    Identity data_name_id(RandomString(64));
    MaidManager::Key key(public_maid_.name(), data_name_id, ImmutableData::Tag::kValue);
    Commit(key, ActionMaidManagerPut(kTestChunkSize));
    EXPECT_NO_THROW(Get(key));
    chunks_list.push_back(ImmutableData::Name(data_name_id));
    keys_list.push_back(key);
  }

  for (size_t index(0); index < kChunksCount; ++index)
    Commit(keys_list[index], ActionMaidManagerIncrementReferenceCount());

  for (size_t index(0); index < kChunksCount; ++index) {
    auto value(Get(keys_list[index]));
    EXPECT_TRUE(value.count() == 2);
  }

  ActionMaidManagerDecrementReferenceCounts action_decrement_reference(
      (nfs_vault::DataNames(chunks_list)));
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
      CreateGroupUnresolvedAction<MaidManager::UnresolvedDecrementReferenceCounts>(
          metadata_key, action_decrement_reference, group_source));
  SendSync<MaidManager::UnresolvedDecrementReferenceCounts>(group_unresolved_action, group_source);
  for (size_t index(0); index < kChunksCount; ++index) {
    auto value(Get(keys_list[index]));
    EXPECT_TRUE(value.count() == 1);
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
