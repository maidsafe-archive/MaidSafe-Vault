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

#include "maidsafe/vault/version_handler/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {


class VersionHandlerServiceTest {
 public:
  VersionHandlerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      version_handler_service_(pmid_, routing_),
      asio_service_(2) {
    asio_service_.Start();
  }

  VersionHandler::Value Get(const VersionHandler::Key& key) {
    return version_handler_service_.db_.Get(key);
  }

  void Store(const VersionHandler::Key& key, const ActionVersionHandlerPut& action) {
    version_handler_service_.db_.Commit(key, action);
  }

  template <typename UnresilvedActionType>
  void SendSync(const std::vector<UnresilvedActionType>& unresolved_actions,
                const std::vector<routing::GroupSource>& group_source);

 protected:
  passport::Pmid pmid_;
  routing::Routing routing_;
  VersionHandlerService version_handler_service_;
  AsioService asio_service_;
};

template <typename UnresilvedActionType>
void VersionHandlerServiceTest::SendSync(
    const std::vector<UnresilvedActionType>& /*unresolved_actions*/,
    const std::vector<routing::GroupSource>& /*group_source*/) {
  UnresilvedActionType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void VersionHandlerServiceTest::SendSync<VersionHandler::UnresolvedPutVersion>(
         const std::vector<VersionHandler::UnresolvedPutVersion>& unresolved_actions,
         const std::vector<routing::GroupSource>& group_source) {
  AddLocalActionAndSendGroupActions<VersionHandlerService, VersionHandler::UnresolvedPutVersion,
                                    SynchroniseFromVersionHandlerToVersionHandler>(
      &version_handler_service_, version_handler_service_.sync_put_versions_, unresolved_actions,
      group_source);
}

TEST_CASE_METHOD(VersionHandlerServiceTest,
                 "version handler: checking handler is available for all message types",
                 "[CheckHandler]") {
  SECTION("GetVersionsRequestFromMaidNodeToVersionHandler") {
    routing::SingleSource maid_node((NodeId(NodeId::kRandomId)));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(CreateContent<nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Contents>());
    auto get_version(CreateMessage<nfs::GetVersionsRequestFromMaidNodeToVersionHandler>(content));
    SingleSendsToGroup(&version_handler_service_, get_version, maid_node, version_group_id);
  }

  SECTION("GetVersionsRequestFromDataGetterToVersionHandler") {
    routing::SingleSource data_getter_id((NodeId(NodeId::kRandomId)));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(CreateContent<nfs::GetVersionsRequestFromDataGetterToVersionHandler::Contents>());
    auto get_version(CreateMessage<nfs::GetVersionsRequestFromDataGetterToVersionHandler>(content));
    SingleSendsToGroup(&version_handler_service_, get_version, data_getter_id, version_group_id);
  }

  SECTION("GetBranchRequestFromMaidNodeToVersionHandler") {
    routing::SingleSource maid_node((NodeId(NodeId::kRandomId)));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(CreateContent<nfs::GetBranchRequestFromMaidNodeToVersionHandler::Contents>());
    auto get_branch(CreateMessage<nfs::GetBranchRequestFromMaidNodeToVersionHandler>(content));
    SingleSendsToGroup(&version_handler_service_, get_branch, maid_node, version_group_id);
  }

  SECTION("GetBranchRequestFromDataGetterToVersionHandler") {
    routing::SingleSource data_getter_id((NodeId(NodeId::kRandomId)));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(CreateContent<nfs::GetBranchRequestFromDataGetterToVersionHandler::Contents>());
    auto get_branch(CreateMessage<nfs::GetBranchRequestFromDataGetterToVersionHandler>(content));
    SingleSendsToGroup(&version_handler_service_, get_branch, data_getter_id, version_group_id);
  }

  SECTION("PutVersionRequestFromMaidManagerToVersionHandler") {
    auto group_source(CreateGroupSource((NodeId(NodeId::kRandomId))));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(CreateContent<PutVersionRequestFromMaidManagerToVersionHandler::Contents>());
    auto put_version(CreateMessage<PutVersionRequestFromMaidManagerToVersionHandler>(content));
    GroupSendToGroup(&version_handler_service_, put_version, group_source, version_group_id);
  }

  SECTION("DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler") {
    auto group_source(CreateGroupSource((NodeId(NodeId::kRandomId))));
    routing::GroupId version_group_id((NodeId(NodeId::kRandomId)));
    auto content(
        CreateContent<DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Contents>());
    auto delete_branch(
        CreateMessage<DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler>(content));
    GroupSendToGroup(&version_handler_service_, delete_branch, group_source, version_group_id);
  }

  SECTION("SynchroniseFromVersionHandlerToVersionHandler") {
    NodeId group_id(NodeId::kRandomId);
    auto group_source(CreateGroupSource(group_id));
    routing::GroupId version_group_id(group_id);
    SynchroniseFromVersionHandlerToVersionHandler::Contents content((std::string()));
    auto sync(CreateMessage<SynchroniseFromVersionHandlerToVersionHandler>(content));
    REQUIRE_THROWS(GroupSendToGroup(&version_handler_service_, sync, group_source,
                                    version_group_id));
  }
}

TEST_CASE_METHOD(VersionHandlerServiceTest,
                 "version handler: checking all sync message types are handled",
                 "[CheckSync]") {
  SECTION("PutVersion") {
    NodeId sender_id(NodeId::kRandomId), originator(NodeId::kRandomId);
    auto content(CreateContent<nfs_vault::DataNameOldNewVersion>());
    nfs::MessageId message_id(RandomInt32());
    VersionHandler::Key key(content.data_name.raw_name, ImmutableData::Tag::kValue,
                            Identity(originator.string()));
    ActionVersionHandlerPut action_put_version(content.old_version_name, content.new_version_name,
                                               sender_id, message_id);
    auto group_source(CreateGroupSource(NodeId(content.data_name.raw_name.string())));
    auto group_unresolved_action(
             CreateGroupUnresolvedAction<VersionHandler::UnresolvedPutVersion>(
                 key, action_put_version, group_source));
    SendSync<VersionHandler::UnresolvedPutVersion>(group_unresolved_action, group_source);
    REQUIRE_NOTHROW(Get(key));
  }

  SECTION("DeleteBranchUntilFork") {}
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
