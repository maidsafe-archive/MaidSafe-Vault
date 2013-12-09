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

class PmidManagerServiceTest {
 public:
  PmidManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      pmid_manager_service_(pmid_, routing_) {}

  template <typename UnresolvedActionType>
  std::vector<std::unique_ptr<UnresolvedActionType>> GetUnresolvedActions();

  void AddGroup(const PmidManager::GroupName& group_name,
                const PmidManager::Metadata& metadata) {
    pmid_manager_service_.group_db_.AddGroup(group_name, metadata);
  }

  PmidManager::Metadata GetMetadata(const PmidManager::GroupName& group_name) {
    return pmid_manager_service_.group_db_.GetMetadata(group_name);
  }

  PmidManager::Value GetValue(const PmidManager::Key& key) {
    return pmid_manager_service_.group_db_.GetValue(key);
  }

  template <typename ActionType>
  void Commit(const PmidManager::Key& key, const ActionType& action) {
    pmid_manager_service_.group_db_.Commit(key, action);
  }

  template <typename UnresolvedActionType>
  void SendSync(const std::vector<UnresolvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  passport::Pmid pmid_;
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
      &pmid_manager_service_, pmid_manager_service_.sync_deletes_, unresolved_actions, group_source);
}

template <typename UnresolvedActionType>
std::vector<std::unique_ptr<UnresolvedActionType>>
PmidManagerServiceTest::GetUnresolvedActions() {
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

TEST_CASE_METHOD(PmidManagerServiceTest,
                 "pmid manager: check handlers for all messages are available",
                 "[Handler][PmidManager][Service]") {
  SECTION("PutRequestFromDataManagerToPmidManager") {
    auto content(CreateContent<PutRequestFromDataManagerToPmidManager::Contents>());
    auto put_request(CreateMessage<PutRequestFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(NodeId(put_request.contents->name.raw_name.string())));
    CHECK_NOTHROW(GroupSendToGroup(&pmid_manager_service_, put_request, group_source,
                                   routing::GroupId(this->routing_.kNodeId())));
    CHECK(GetUnresolvedActions<PmidManager::UnresolvedPut>().size() == 1);
  }

  SECTION("PutFailureFromPmidNodeToPmidManager") {
    PmidManagerMetadata metadata(PmidName(pmid_.name()));
    metadata.claimed_available_size = kTestChunkSize * 100;
    AddGroup(PmidName(pmid_.name()), PmidManagerMetadata());
    auto content(CreateContent<PutFailureFromPmidNodeToPmidManager::Contents>());
    auto put_failure(CreateMessage<PutFailureFromPmidNodeToPmidManager>(content));
    CHECK_NOTHROW(SingleSendsToGroup(&pmid_manager_service_, put_failure,
                                     routing::SingleSource(NodeId(NodeId::kRandomId)),
                                     routing::GroupId(NodeId(pmid_.name()->string()))));
    CHECK(GetUnresolvedActions<PmidManager::UnresolvedDelete>().size() == 1);
    metadata = GetMetadata(PmidName(pmid_.name()));
    CHECK(metadata.claimed_available_size == 0);
  }

  SECTION("DeleteRequestFromDataManagerToPmidManager") {
    auto content(CreateContent<DeleteRequestFromDataManagerToPmidManager::Contents>());
    auto delete_request(CreateMessage<DeleteRequestFromDataManagerToPmidManager>(content));
    auto group_source(CreateGroupSource(NodeId(content.raw_name.string())));
    CHECK_NOTHROW(GroupSendToGroup(&pmid_manager_service_, delete_request, group_source,
                                   routing::GroupId(NodeId(pmid_.name()->string()))));
    CHECK(GetUnresolvedActions<PmidManager::UnresolvedDelete>().size() == 1);
  }

  SECTION("GetPmidAccountRequestFromPmidNodeToPmidManager") {
    auto content(CreateContent<GetPmidAccountRequestFromPmidNodeToPmidManager::Contents>());
    auto get_pmid_account_request(CreateMessage<GetPmidAccountRequestFromPmidNodeToPmidManager>(
                                      content));
    CHECK_NOTHROW(SingleSendsToGroup(&pmid_manager_service_, get_pmid_account_request,
                                     routing::SingleSource(NodeId(pmid_.name()->string())),
                                     routing::GroupId(NodeId(pmid_.name()->string()))));
  }

  SECTION("PmidHealthRequestFromMaidNodeToPmidManager") {
    auto content(CreateContent<PmidHealthRequestFromMaidNodeToPmidManager::Contents>());
    auto get_pmid_account_request(CreateMessage<PmidHealthRequestFromMaidNodeToPmidManager>(
                                      content));
    NodeId maid_node(NodeId::kRandomId);
    CHECK_NOTHROW(SingleSendsToGroup(&pmid_manager_service_, get_pmid_account_request,
                                     routing::SingleSource(maid_node),
                                     routing::GroupId(NodeId(pmid_.name()->string()))));
  }
}

TEST_CASE_METHOD(PmidManagerServiceTest,
                 "pmid manager: check handlers availability", "[Sync][PmidManager][Service]") {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  auto group_source(CreateGroupSource(NodeId(pmid_.name()->string())));
  PmidManager::Key key(PmidName(pmid_.name()), data.name(), ImmutableData::Tag::kValue);
  AddGroup(PmidName(pmid_.name()), PmidManagerMetadata());

  SECTION("Put") {
    ActionPmidManagerPut action_put(kTestChunkSize, nfs::MessageId(RandomInt32()));
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<PmidManager::UnresolvedPut>(key, action_put,
                                                                     group_source));
    SendSync<PmidManager::UnresolvedPut>(group_unresolved_action, group_source);
    auto value(GetValue(key));
    auto metadata(GetMetadata(PmidName(pmid_.name())));
    CHECK(value.size() == kTestChunkSize);
    CHECK(metadata.stored_total_size == kTestChunkSize);
    CHECK(metadata.stored_count == 1);
  }

  SECTION("Delete") {
    ActionPmidManagerDelete action_delete(false);
    Commit(key, ActionPmidManagerPut(kTestChunkSize, nfs::MessageId(RandomInt32())));
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<PmidManager::UnresolvedDelete>(key, action_delete,
                                                                        group_source));
    SendSync<PmidManager::UnresolvedDelete>(group_unresolved_action, group_source);
    CHECK_THROWS(GetValue(key));
    CHECK_NOTHROW(GetMetadata(PmidName(pmid_.name())));
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
