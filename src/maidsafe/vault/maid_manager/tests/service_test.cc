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

#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {

namespace vault {

namespace test {

class MaidManagerServiceTest {
 public:
  MaidManagerServiceTest() :
      anmaid_(),
      maid_(anmaid_),
      pmid_(maid_),
      public_maid_(maid_),
      routing_(pmid_),
      data_getter_(asio_service_, routing_, std::vector<passport::PublicPmid>()),
      maid_manager_service_(pmid_, routing_, data_getter_),
      asio_service_(2) {
    asio_service_.Start();
  }

  NodeId MaidNodeId() {
    return NodeId(maid_.name()->string());
  }

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
    maid_manager_service_.group_db_.Commit(public_maid_.name(),
                                           ActionMaidManagerRegisterPmid(pmid_registration));
  }

  template <typename UnresilvedActionType>
  void SendSync(const std::vector<UnresilvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

  bool Equal(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs) {
    if (lhs.total_put_data_ != rhs.total_put_data_)
      return false;

    if (lhs.pmid_totals_.size() != rhs.pmid_totals_.size())
      return false;

    for (const auto& pmid_total : lhs.pmid_totals_) {
      auto found(std::find_if(std::begin(rhs.pmid_totals_), std::end(rhs.pmid_totals_),
                              [&](const PmidTotals& rhs_pmid_total) {
                                return pmid_total == rhs_pmid_total;
                              }));
       if (found == std::end(rhs.pmid_totals_))
         return false;
    }
    return true;
  }

 protected:
  passport::Anmaid anmaid_;
  passport::Maid maid_;
  passport::Pmid pmid_;
  passport::PublicMaid public_maid_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  MaidManagerService maid_manager_service_;
  AsioService asio_service_;
};

template <typename UnresilvedActionType>
void  MaidManagerServiceTest::SendSync(
    const std::vector<UnresilvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresilvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
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
      &maid_manager_service_, maid_manager_service_.sync_puts_, unresolved_actions,
      group_source);
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

TEST_CASE_METHOD(MaidManagerServiceTest, "put request from maid node", "[PutRequestFromMaidNode]") {
  CreateAccount();
  auto content(CreateContent<nfs::PutRequestFromMaidNodeToMaidManager::Contents>());
  auto put_request(CreateMessage<nfs::PutRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, put_request, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "put response from data manager",
                 "[PutResponseFromDataManager]") {
  auto content(CreateContent<PutResponseFromDataManagerToMaidManager::Contents>());
  NodeId data_name_id(content.name.raw_name.string());
  auto put_response(CreateMessage<PutResponseFromDataManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(data_name_id));
  GroupSendToGroup(&maid_manager_service_, put_response, group_source,
                   routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "put failure from data manager",
                 "[PutFailureFromDataManager]") {
  auto content(CreateContent<PutFailureFromDataManagerToMaidManager::Contents>());
  NodeId data_name_id(content.name.raw_name.string()), maid_node_id(NodeId::kRandomId);
  auto put_failure(CreateMessage<PutFailureFromDataManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(data_name_id));
  GroupSendToGroup(&maid_manager_service_, put_failure, group_source,
                   routing::GroupId(maid_node_id));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "delete request from maid node",
                 "[DeleteRequestFromMaidNode]") {
  CreateAccount();
  auto content(CreateContent<nfs::DeleteRequestFromMaidNodeToMaidManager::Contents>());
  auto delete_request(CreateMessage<nfs::DeleteRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, delete_request, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "delete branch until fork request from maid node"
                 "[DeleteBranchUntilForkRequestFromMaidNode]") {
  auto content(
           CreateContent<nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Contents>());
  auto delete_branch(
           CreateMessage<nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, delete_branch, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "create account request from maid node",
                "[CreateAccountRequestFromMaidNode]") {  
  nfs::CreateAccountRequestFromMaidNodeToMaidManager::Contents content(
      (passport::PublicMaid(maid_)), (passport::PublicAnmaid(anmaid_)));
  auto create_account(CreateMessage<nfs::CreateAccountRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, create_account, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "remove account request from maid node",
                 "[RemoveAccountRequestFromMaidNode]") {
  nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Contents content(anmaid_);
  auto remove_account(CreateMessage<nfs::RemoveAccountRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, remove_account, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "register pmid request from maid node",
                 "[RegisterPmidRequestFromMaidNode]") {
  nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Contents content(maid_, pmid_, true);
  auto register_pmid(CreateMessage<nfs::RegisterPmidRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, register_pmid, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "unregister pmid request from maid node",
                                         "[UnregisterPmidRequestFromMaidNode]") {
  nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Contents content(maid_, pmid_, false);
  auto unregister_pmid(CreateMessage<nfs::UnregisterPmidRequestFromMaidNodeToMaidManager>(content));
  SingleSendsToGroup(&maid_manager_service_, unregister_pmid, routing::SingleSource(MaidNodeId()),
                     routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "pmid health response from pmid manager",
                "[PmidHealthResponseFromPmidManager]") {
  PmidName pmid_name(pmid_.name());
  PmidManagerMetadata pmid_manager_metadata(pmid_name);
  pmid_manager_metadata.SetAvailableSize(kTestChunkSize * (RandomUint32() % kAverageChunksStored));
  PmidHealthResponseFromPmidManagerToMaidManager::Contents content(
      nfs_vault::PmidHealth(pmid_manager_metadata.Serialise()),
      nfs_client::ReturnCode(CommonErrors::success));
  auto pmid_health_response(CreateMessage<PmidHealthResponseFromPmidManagerToMaidManager>(content));
  auto group_source(CreateGroupSource(NodeId(pmid_name->string())));
  GroupSendToGroup(&maid_manager_service_, pmid_health_response, group_source,
                   routing::GroupId(MaidNodeId()));
  // TO BE CONTINUED
}

TEST_CASE_METHOD(MaidManagerServiceTest, "creat account request sync from maid manager",
                 "[CreateAccountSynchroniseFromMaidManager]") {
  nfs::MessageId message_id(RandomInt32());
  ActionCreateAccount action_create_account(message_id);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedCreateAccount>(
               metadata_key, action_create_account, group_source));
  SendSync<MaidManager::UnresolvedCreateAccount>(group_unresolved_action, group_source);
  REQUIRE_NOTHROW(GetMetadata(public_maid_.name()));
}

TEST_CASE_METHOD(MaidManagerServiceTest, "remove account sync from maid manager"
                 "[RemoveAccountSynchroniseFromMaidManager]") {
  CreateAccount();
  ActionRemoveAccount action_remove_account((nfs::MessageId(RandomUint32())));
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedRemoveAccount>(
               metadata_key, action_remove_account, group_source));
  SendSync<MaidManager::UnresolvedRemoveAccount>(group_unresolved_action, group_source);
  REQUIRE_THROWS(GetMetadata(public_maid_.name()));
}

TEST_CASE_METHOD(MaidManagerServiceTest, "put sync from maid manager",
                 "[PutSynchroniseFromMaidManager]") {
  CreateAccount();
  ActionMaidManagerPut action_put(kTestChunkSize);
  MaidManager::Key key(public_maid_.name(), Identity(RandomString(64)), ImmutableData::Tag::kValue);
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedPut>(key, action_put, group_source));
  SendSync<MaidManager::UnresolvedPut>(group_unresolved_action, group_source);
}

TEST_CASE_METHOD(MaidManagerServiceTest, "delete sync from maid manager",
                 "[DeleteSynchroniseFromMaidManager]") {
  CreateAccount();
  ActionMaidManagerDelete action_delete((nfs::MessageId(RandomInt32())));
  Identity data_name_id(RandomString(64));
  MaidManager::Key key(public_maid_.name(), data_name_id, ImmutableData::Tag::kValue);
  Commit(key, ActionMaidManagerPut(kTestChunkSize));
  REQUIRE_NOTHROW(Get(key));
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedDelete>(key, action_delete,
                                                                      group_source));
  SendSync<MaidManager::UnresolvedDelete>(group_unresolved_action, group_source);
  REQUIRE_THROWS(Get(key));
}

TEST_CASE_METHOD(MaidManagerServiceTest, "register pmid sync from maid manager",
                 "[RegistedPmidSynchroniseFromMaidManager]") {
  CreateAccount();
  nfs_vault::PmidRegistration pmid_registration(maid_, pmid_, false);
  ActionMaidManagerRegisterPmid action_register_pmid(pmid_registration);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedRegisterPmid>(
               metadata_key, action_register_pmid, group_source));
  SendSync<MaidManager::UnresolvedRegisterPmid>(group_unresolved_action, group_source);
  MaidManager::Metadata metadata(GetMetadata(public_maid_.name()));
  CHECK(MetadataPmidTotals(metadata).size() == 1); // FAILS BECAUSE DATAGETTER GET NEVER SUCCEEDS
}

TEST_CASE_METHOD(MaidManagerServiceTest, "unregister pmid sync from maid manager",
                 "[UnregistedPmidSynchroniseFromMaidManager]") {
  CreateAccount();
  RegisterPmid();
  MaidManager::Metadata metadata(GetMetadata(public_maid_.name()));
  CHECK(MetadataPmidTotals(metadata).size() == 1);
  nfs_vault::PmidRegistration pmid_unregistration(maid_, pmid_, true);
  ActionMaidManagerUnregisterPmid action_unregister_pmid(pmid_unregistration);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedUnregisterPmid>(
               metadata_key, action_unregister_pmid, group_source));
  SendSync<MaidManager::UnresolvedUnregisterPmid>(group_unresolved_action, group_source);
  metadata = GetMetadata(public_maid_.name());
  CHECK(MetadataPmidTotals(metadata).size() == 0);
}

TEST_CASE_METHOD(MaidManagerServiceTest, "update pmid sync form maid manager",
                 "[UpdatePmidSynchroniseFromMaidManager]") {
  PmidManagerMetadata pmid_manager_metadata(PmidName(Identity(pmid_.name()->string())));
  pmid_manager_metadata.stored_count = 1;
  pmid_manager_metadata.stored_total_size = kTestChunkSize;
  pmid_manager_metadata.lost_count = 2;
  pmid_manager_metadata.lost_total_size =  2 * kTestChunkSize;
  pmid_manager_metadata.claimed_available_size = 2 << 20;
  PmidTotals pmid_totals(std::string(), pmid_manager_metadata);
  MaidManager::Metadata metadata(100, std::vector<PmidTotals>({ pmid_totals }));
  AddMetadata(public_maid_.name(), metadata);
  pmid_manager_metadata.stored_count = 4;
  pmid_manager_metadata.stored_total_size = kTestChunkSize * 4;
  pmid_manager_metadata.lost_count = 6;
  pmid_manager_metadata.lost_total_size =  6 * kTestChunkSize;
  pmid_manager_metadata.claimed_available_size = 2 << 10;
  PmidTotals updated_pmid_totals(std::string(), pmid_manager_metadata);
  MaidManager::Metadata updated_metadata(100, std::vector<PmidTotals>({ updated_pmid_totals }));
  ActionMaidManagerUpdatePmidHealth action_update_pmid_health(pmid_manager_metadata);
  MaidManager::MetadataKey metadata_key(public_maid_.name());
  auto group_source(CreateGroupSource(MaidNodeId()));
  auto group_unresolved_action(
           CreateGroupUnresolvedAction<MaidManager::UnresolvedUpdatePmidHealth>(
               metadata_key, action_update_pmid_health, group_source));
  SendSync<MaidManager::UnresolvedUpdatePmidHealth>(group_unresolved_action, group_source);
  try {
    MaidManager::Metadata stored_metadata(GetMetadata(public_maid_.name()));
    CHECK(Equal(stored_metadata, updated_metadata));
  }
  catch (...) {
    CHECK_FALSE(true);
  }
}

TEST_CASE_METHOD(MaidManagerServiceTest, "account transfer from maid manager",
                 "[AccountTransferFromMaidManager]") {
  // Not implemented yet
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
