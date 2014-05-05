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

class CacheHandlerTest : public VaultNetwork  {
 public:
  CacheHandlerTest() {}
};

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByGetResponseToMaidNode) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData data(NonEmptyString(RandomString(1024)));
  NodeId random_id(NodeId::kRandomId);
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetResponseFromDataManagerToMaidNode NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::seconds(1));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::GroupSource(routing::GroupId(NodeId(data.name().value.string())),
                           routing::SingleId(kNodeId(0))),
      NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string())
                << " id" << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  Send(0, message);
  Sleep(std::chrono::seconds(3));

  LOG(kVerbose) << "Get attempt";

  auto future(clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5)));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }
}

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByCachedResponseToMaidNode) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData data(NonEmptyString(RandomString(1024)));
  NodeId random_id(NodeId::kRandomId);
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetCachedResponseFromCacheHandlerToMaidNode NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::seconds(1));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::SingleSource(routing::SingleId(kNodeId(0))),
      NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string())
                << " id" << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  Send(0, message);
  Sleep(std::chrono::seconds(3));

  LOG(kVerbose) << "Get attempt";

  auto future(clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5)));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }
}

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByGetResponseToDataGetter) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData data(NonEmptyString(RandomString(1024)));
  NodeId random_id(NodeId::kRandomId);
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetResponseFromDataManagerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::seconds(1));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::GroupSource(routing::GroupId(NodeId(data.name().value.string())),
                           routing::SingleId(kNodeId(0))),
      NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string())
                << " id" << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  Send(0, message);
  Sleep(std::chrono::seconds(3));

  LOG(kVerbose) << "Get attempt";

  auto future(clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5)));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }
}

TEST_F(CacheHandlerTest, FUNC_GetFromCacheStoredByCachedResponseToDataGetter) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData data(NonEmptyString(RandomString(1024)));
  NodeId random_id(NodeId::kRandomId);
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::seconds(1));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(data));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::SingleSource(routing::SingleId(kNodeId(0))),
      NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(data.name().value.string())
                << " id" << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  Send(0, message);
  Sleep(std::chrono::seconds(3));

  LOG(kVerbose) << "Get attempt";

  auto future(clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5)));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }
}

TEST_F(CacheHandlerTest, FUNC_NonCacheableData) {
  EXPECT_TRUE(AddClient(true));
  passport::Anmaid anmaid;
  passport::PublicAnmaid public_anmaid(anmaid);
  NodeId random_id(NodeId::kRandomId);
  nfs::MessageId message_id(RandomUint32());
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter NfsMessage;
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  Sleep(std::chrono::seconds(1));

  NfsMessage nfs_message(message_id, NfsMessage::Contents(public_anmaid));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::SingleSource(routing::SingleId(kNodeId(0))),
      NfsMessage::Receiver(random_id), routing::Cacheable::kPut);
  LOG(kVerbose) << "To be cached: " << HexSubstr(anmaid.name().value.string())
                << " id" << message_id;
  // Caching on all nodes in the network.
  routing::Parameters::max_route_history = kNetworkSize;
  Send(0, message);
  Sleep(std::chrono::seconds(3));

  LOG(kVerbose) << "Get attempt";

  auto future(clients_[0]->nfs_->Get<passport::PublicAnmaid::Name>(public_anmaid.name(),
                                                                   std::chrono::seconds(5)));
  try {
    future.get();
    EXPECT_TRUE(false) << "Should have thrown: " << DebugId(NodeId(anmaid.name()->string()));
  }
  catch (...) {
    EXPECT_TRUE(true);
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

