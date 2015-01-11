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

#include "maidsafe/vault/vault.h"

#include <functional>
#include <memory>

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/passport/passport.h"

#include "maidsafe/vault/tests/hybrid_network.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

class VaultTest : public testing::Test {
 public:
  VaultTest() : env_(HybridEnvironment::g_environment()) {}

 protected:
  std::shared_ptr<HybridNetwork> env_;
};


TEST_F(VaultTest, FUNC_Constructor) {
  EXPECT_TRUE(env_->AddVault());
}

TEST_F(VaultTest, BEH_HandleDataManagerMessage) {
  using VaultMessage = vault::PutRequestFromMaidManagerToDataManager;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize)));
  auto manager_index(env_->ManagerIndex(maid_node_id));
  auto data(env_->CreateDataForManager(env_->public_pmids().back().name()));
  RoutingMessage routing_message(
                     CreateMessage<VaultMessage>(VaultMessage::Contents(data)).Serialise(),
                     routing::GroupSource(routing::GroupId(maid_node_id),
                                          routing::SingleId(
                                              env_->nodes_.at(manager_index)->node_id())),
                     routing::GroupId(NodeId(data.name()->string()))); 
}

TEST_F(VaultTest, BEH_HandleInvalidMessage) {
  using VaultMessage = vault::PutRequestFromMaidManagerToDataManager;
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  auto maid_node_id(NodeId(RandomString(NodeId::kSize)));
  auto manager_index(env_->ManagerIndex(maid_node_id));
  auto data(env_->CreateDataForManager(env_->public_pmids().back().name()));
  RoutingMessage routing_message(
                     CreateMessage<VaultMessage>(VaultMessage::Contents(data)).Serialise(),
                     routing::GroupSource(routing::GroupId(maid_node_id),
                                          routing::SingleId(
                                              env_->nodes_.at(manager_index)->node_id())),
                     routing::GroupId(NodeId(data.name()->string())));
}


}  // namespace test

}  // namespace vault

}  // namespace maidsafe
