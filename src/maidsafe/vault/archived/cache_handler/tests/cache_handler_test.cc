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
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/tests/vault_network.h"

namespace maidsafe {

namespace vault {

namespace test {

class CacheHandlerTest : public testing::Test {
 public:
  CacheHandlerTest() : env_(VaultEnvironment::g_environment()) {
    routing::Parameters::caching = true;
  }

  std::vector<VaultNetwork::ClientPtr>& GetClients() { return env_->clients_; }

 protected:
  std::shared_ptr<VaultNetwork> env_;
};

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByGetResponseToDataGetter) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  NodeId random_id(RandomString(identity_size));
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetResponseFromDataManagerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));

  for (size_t index(0); index < 20; ++index) {
    RoutingMessage message(
        nfs_message.Serialise(),
        routing::GroupSource(routing::GroupId(NodeId(data.name().value.string())),
                             routing::SingleId(env_->kNodeId(0))),
        NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
    LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string()) << " id"
                  << message_id;
    // Caching on all nodes in the network.
    routing::Parameters::max_route_history = kNetworkSize;
    env_->Send(0, message);
    Sleep(std::chrono::milliseconds(300));
    random_id = NodeId(RandomString(identity_size));
  }
  LOG(kVerbose) << "Get attempt";
  EXPECT_NO_THROW(env_->Get<ImmutableData>(data.name()))
      << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
}

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByCachedResponseToDataGetter) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  NodeId random_id(RandomString(identity_size));
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::milliseconds(300));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));
  for (size_t index(0); index < 30; ++index) {
    RoutingMessage message(nfs_message.Serialise(),
                           routing::SingleSource(routing::SingleId(env_->kNodeId(0))),
                           NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
    LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string()) << " id"
                  << message_id;
    // Caching on all nodes in the network.
    routing::Parameters::max_route_history = kNetworkSize;
    env_->Send(0, message);
    Sleep(std::chrono::milliseconds(300));
    random_id = NodeId(RandomString(identity_size));
  }
  LOG(kVerbose) << "Get attempt";
  EXPECT_NO_THROW(env_->Get<ImmutableData>(data.name()))
      << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
}

TEST_F(CacheHandlerTest, FUNC_NonCacheableData) {
  passport::Anmaid anmaid;
  passport::PublicAnmaid public_anmaid(anmaid);
  NodeId random_id(RandomString(identity_size));
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  NfsMessage nfs_message(message_id, NfsMessage::Contents(public_anmaid));
  RoutingMessage message(nfs_message.Serialise(),
                         routing::SingleSource(routing::SingleId(env_->kNodeId(0))),
                         NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(anmaid.name().value.string()) << " id"
                << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  env_->Send(0, message);

  LOG(kVerbose) << "Get attempt";
  EXPECT_THROW(env_->Get<passport::PublicAnmaid>(public_anmaid.name()), std::exception)
      << "Failed to retrieve: " << HexSubstr(public_anmaid.name()->string());
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
