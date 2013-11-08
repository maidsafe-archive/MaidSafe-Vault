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

typedef PutRequestFromDataManagerToPmidManager PutRequest;
typedef PutFailureFromPmidNodeToPmidManager PutFailure;
typedef DeleteRequestFromDataManagerToPmidManager DeleteRequest;
typedef GetPmidAccountRequestFromPmidNodeToPmidManager GetPmidAccount;
typedef PmidHealthRequestFromMaidNodeToPmidManager PmidHealthRequest;


class PmidManagerServiceTest  : public testing::Test {
 public:
  PmidManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      pmid_manager_service_(pmid_, routing_) {}

 protected:
  passport::Maid MakeMaid() {
    passport::Anmaid anmaid;
    return passport::Maid(anmaid);
  }

  passport::Pmid MakePmid() { return passport::Pmid(MakeMaid()); }

  passport::PublicPmid MakePublicPmid() {
    passport::Pmid pmid(MakePmid());
    return passport::PublicPmid(pmid);
  }

  passport::Pmid pmid_;
  routing::Routing routing_;
  PmidManagerService pmid_manager_service_;
};

TEST_F(PmidManagerServiceTest, BEH_PutSync) {
  PmidManager::Key group_key(PmidManager::GroupName(Identity(NodeId(NodeId::kRandomId).string())),
                       Identity(NodeId(NodeId::kRandomId).string()),
                       ImmutableData::Tag::kValue);
  NodeId group_id(NodeId::kRandomId);
  std::vector<routing::GroupSource> group_sources;
  group_sources.push_back(routing::GroupSource(routing::GroupId(group_id),
                                               routing::SingleId(this->routing_.kNodeId())));
  for (int index = 1; index < routing::Parameters::node_group_size; ++index)
    group_sources.push_back(routing::GroupSource(routing::GroupId(group_id),
                                                 routing::SingleId(NodeId(NodeId::kRandomId))));

  nfs::MessageId message_id(RandomUint32());
  int32_t size(1024);
  this->pmid_manager_service_.group_db_.AddGroup(group_key.group_name(), PmidManagerMetadata());
  for (int index = 0; index < routing::Parameters::node_group_size; ++index) {
    ActionPmidManagerPut action_put(size, message_id);
    PmidManager::UnresolvedPut unresolved_action(group_key, action_put,
                                                 group_sources.at(index).sender_id.data);
    protobuf::Sync proto_sync;
    proto_sync.set_action_type(static_cast<int>(ActionPmidManagerPut::kActionId));
    proto_sync.set_serialised_unresolved_action(unresolved_action.Serialise());
    SynchroniseFromPmidManagerToPmidManager message(
        message_id, nfs_vault::Content(proto_sync.SerializeAsString()));
    if (index == 0) {
      this->pmid_manager_service_.sync_puts_.AddLocalAction(unresolved_action);
    } else {
      this->pmid_manager_service_.HandleMessage(message, group_sources.at(index),
                                                routing::GroupId(group_id));
    }

    if (index == routing::Parameters::node_group_size - 1) {
      EXPECT_EQ(this->pmid_manager_service_.sync_puts_.GetUnresolvedActions().size(), 0);
      return;
    }

    EXPECT_EQ(this->pmid_manager_service_.sync_puts_.GetUnresolvedActions().size(), 1);
    EXPECT_EQ(
        this->pmid_manager_service_.sync_puts_.GetUnresolvedActions()[0]->peer_and_entry_ids.size(),
        index);
  }
}

TEST_F(PmidManagerServiceTest, BEH_PutRequestFromDataManager) {
  auto content(CreateContent<PutRequest::Contents>());
  auto put_request(CreateMessage<PutRequest>(content));
  routing::GroupSource group_source(
      routing::GroupId(NodeId(put_request.contents->name.raw_name.string())),
      routing::SingleId(NodeId(NodeId::kRandomId)));
  this->pmid_manager_service_.HandleMessage(put_request, group_source,
                                            routing::GroupId(this->routing_.kNodeId()));
  EXPECT_EQ(this->pmid_manager_service_.sync_puts_.GetUnresolvedActions().size(), 1);
}

TEST_F(PmidManagerServiceTest, BEH_PutFailureFromPmidNode) {
  auto content(CreateContent<PutFailure::Contents>());
  auto put_failure(CreateMessage<PutFailure>(content));
  this->pmid_manager_service_.HandleMessage(put_failure,
                                            routing::SingleSource(NodeId(NodeId::kRandomId)),
                                            routing::GroupId(this->routing_.kNodeId()));
  EXPECT_EQ(this->pmid_manager_service_.sync_deletes_.GetUnresolvedActions().size(), 1);
}

TEST_F(PmidManagerServiceTest, BEH_DeleterequestFromDataManager) {
  auto content(CreateContent<DeleteRequest::Contents>());
  auto delete_request(CreateMessage<DeleteRequest>(content));
  auto group_source(CreateGroupSource(NodeId(content.raw_name.string())));
  NodeId pmid_node(NodeId::kRandomId);
  for (u_int32_t index(0); index < group_source.size(); ++index) {
    this->pmid_manager_service_.HandleMessage(delete_request, group_source[index],
                                              routing::GroupId(pmid_node));
  }
  EXPECT_EQ(this->pmid_manager_service_.sync_deletes_.GetUnresolvedActions().size(), 1);
}

TEST_F(PmidManagerServiceTest, BEH_GetPmidAccountRequestFromPmidNode) {
  auto content(CreateContent<GetPmidAccount::Contents>());
  auto get_pmid_account_request(CreateMessage<GetPmidAccount>(content));
  NodeId pmid_node(NodeId::kRandomId);
  this->pmid_manager_service_.HandleMessage(get_pmid_account_request,
                                            routing::SingleSource(pmid_node),
                                            routing::GroupId(pmid_node));
}

TEST_F(PmidManagerServiceTest, BEH_PmidHealthRequestFromMaidNode) {
  auto content(CreateContent<PmidHealthRequest::Contents>());
  auto get_pmid_account_request(CreateMessage<PmidHealthRequest>(content));
  NodeId pmid_node(NodeId::kRandomId), maid_node(NodeId::kRandomId);
  this->pmid_manager_service_.HandleMessage(get_pmid_account_request,
                                            routing::SingleSource(maid_node),
                                            routing::GroupId(pmid_node));
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
