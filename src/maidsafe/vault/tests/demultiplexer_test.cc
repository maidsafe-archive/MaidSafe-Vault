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

#include "maidsafe/vault/demultiplexer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/passport/passport.h"

#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/mpid_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class DemultiplexerTest : public testing::Test {
 public:
  DemultiplexerTest() :
      kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault" +
                                                RandomAlphaNumericString(8))),
      vault_dir_(*kTestRoot_),
      pmid_(passport::CreatePmidAndSigner().first),
      asio_service_(2),
      routing_(new routing::Routing(pmid_)),
      data_getter_(asio_service_, *routing_),
      maid_manager_service_(std::move(std::unique_ptr<MaidManagerService>(new MaidManagerService(
          pmid_, *routing_, data_getter_, vault_dir_)))),
      version_handler_service_(std::move(std::unique_ptr<VersionHandlerService>(
          new VersionHandlerService(pmid_, *routing_, vault_dir_)))),
      data_manager_service_(std::move(std::unique_ptr<DataManagerService>(new DataManagerService(
          pmid_, *routing_, data_getter_, vault_dir_)))),
      pmid_manager_service_(std::move(std::unique_ptr<PmidManagerService>(
          new PmidManagerService(pmid_, *routing_)))),
      pmid_node_service_(std::move(std::unique_ptr<PmidNodeService>(
          new PmidNodeService(pmid_, *routing_, data_getter_, vault_dir_, DiskUsage(100))))),
      mpid_manager_service_(std::move(std::unique_ptr<MpidManagerService>(new MpidManagerService(
          pmid_, *routing_, vault_dir_, DiskUsage(100))))),
      demux_(maid_manager_service_, version_handler_service_, data_manager_service_,
             pmid_manager_service_, pmid_node_service_, mpid_manager_service_, data_getter_) {}

 protected:
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_dir_;
  passport::Pmid pmid_;
  BoostAsioService asio_service_;
  std::unique_ptr<routing::Routing> routing_;
  nfs_client::DataGetter data_getter_;
  nfs::Service<MaidManagerService> maid_manager_service_;
  nfs::Service<VersionHandlerService> version_handler_service_;
  nfs::Service<DataManagerService> data_manager_service_;
  nfs::Service<PmidManagerService> pmid_manager_service_;
  nfs::Service<PmidNodeService> pmid_node_service_;
  nfs::Service<MpidManagerService> mpid_manager_service_;
  Demultiplexer demux_;
};

TEST_F(DemultiplexerTest, BEH_HandleMaidManagerMessage) {
  using NfsMessage = nfs::PutRequestFromMaidNodeToMaidManager;
  using RoutingMessage = routing::Message<NfsMessage::Sender, NfsMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<NfsMessage>(CreateContent<NfsMessage::Contents>()).Serialise(),
      routing::SingleSource(maid_node_id), routing::GroupId(maid_node_id));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandleDataGetterMessage) {
  using NfsMessage = nfs::GetResponseFromDataManagerToDataGetter;
  using RoutingMessage = routing::Message<NfsMessage::Sender, NfsMessage::Receiver>;
  auto data_id(NodeId(RandomString(NodeId::kSize))),
       data_manager_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<NfsMessage>(CreateContent<NfsMessage::Contents>()).Serialise(),
      routing::GroupSource(routing::GroupId(data_id),
                           routing::SingleId(data_manager_id)),
      routing::SingleId(routing_->kNodeId()));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandleDataManagerMessage) {
  using VaultMessage = PutRequestFromMaidManagerToDataManager;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize))),
       maid_manager_id(NodeId(RandomString(NodeId::kSize))),
       data_manager_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<VaultMessage>(CreateContent<VaultMessage::Contents>()).Serialise(),
      routing::GroupSource(routing::GroupId(maid_node_id),
                           routing::SingleId(maid_manager_id)),
      routing::GroupId(data_manager_id));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandlePmidManagerMessage) {
  using VaultMessage = PutRequestFromDataManagerToPmidManager;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto data_id(NodeId(RandomString(NodeId::kSize))),
       data_manager_id(NodeId(RandomString(NodeId::kSize))),
       pmid_manager_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<VaultMessage>(CreateContent<VaultMessage::Contents>()).Serialise(),
      routing::GroupSource(routing::GroupId(data_id),
                           routing::SingleId(data_manager_id)),
      routing::GroupId(pmid_manager_id));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandlePmidNodeMessage) {
  using VaultMessage = PutRequestFromPmidManagerToPmidNode;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto pmid_manager_id(NodeId(RandomString(NodeId::kSize))),
       pmid_node_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<VaultMessage>(CreateContent<VaultMessage::Contents>()).Serialise(),
      routing::GroupSource(routing::GroupId(pmid_node_id),
                           routing::SingleId(pmid_manager_id)),
      routing::SingleId(pmid_node_id));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandleVersionHandlerMessage) {
  using VaultMessage = PutVersionRequestFromMaidManagerToVersionHandler;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize))),
       maid_manager_id(NodeId(RandomString(NodeId::kSize))),
       version_handler_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      CreateMessage<VaultMessage>(CreateContent<VaultMessage::Contents>()).Serialise(),
      routing::GroupSource(routing::GroupId(maid_node_id),
                           routing::SingleId(maid_manager_id)),
      routing::GroupId(version_handler_id));
  demux_.HandleMessage(routing_message);
}

TEST_F(DemultiplexerTest, BEH_HandleInvalidMessage) {
  using VaultMessage = PutVersionRequestFromMaidManagerToVersionHandler;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize))),
       maid_manager_id(NodeId(RandomString(NodeId::kSize))),
       version_handler_id(NodeId(RandomString(NodeId::kSize)));
  RoutingMessage routing_message(
      RandomAlphaNumericString(RandomUint32() % 256),
      routing::GroupSource(routing::GroupId(maid_node_id),
                           routing::SingleId(maid_manager_id)),
      routing::GroupId(version_handler_id));
  demux_.HandleMessage(routing_message);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
