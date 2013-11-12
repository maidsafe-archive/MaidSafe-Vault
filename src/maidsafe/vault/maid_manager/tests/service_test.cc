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
      asio_service_(2) {}

  NodeId MaidNodeId() {
    return NodeId(maid_.name()->string());
  }

  template <typename UnresilvedActionType>
  void Sync(const std::vector<UnresilvedActionType>& unresolved_actions,
            const std::vector<routing::GroupSource>& group_source);

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
void Sync(const std::vector<UnresilvedActionType>& /*unresolved_actions*/,
          const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresilvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void MaidManagerServiceTest::Sync<MaidManager::UnresolvedCreateAccount>(
         const std::vector<MaidManager::UnresolvedCreateAccount>& unresolved_actions,
         const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<MaidManagerService, MaidManager::UnresolvedCreateAccount,
                                    SynchroniseFromMaidManagerToMaidManager>(
      &maid_manager_service_, maid_manager_service_.sync_create_accounts_, unresolved_actions,
      group_source);
}

TEST_CASE_METHOD(MaidManagerServiceTest, "put request from maid node", "[PutRequestFromMaidNode]") {
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
  Sync<MaidManager::UnresolvedCreateAccount>(group_unresolved_action, group_source);
}


TEST_CASE_METHOD(MaidManagerServiceTest, "remove account sync from maid manager"
                 "[RemoveAccountSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "put sync from maid manager",
                 "[PutSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "delete sync from maid manager",
                 "[DeleteSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "register pmid sync from maid manager",
                 "[RegistedPmidSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "unregister pmid sync from maid manager",
                 "[UnregistedPmidSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "update pmid sync form maid manager",
                 "[UpdatePmidSynchroniseFromMaidManager]") {}

TEST_CASE_METHOD(MaidManagerServiceTest, "account transfer from maid manager",
                 "[AccountTransferFromMaidManager]") {}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
