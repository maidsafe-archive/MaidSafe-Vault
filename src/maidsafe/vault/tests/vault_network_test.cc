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

#include "maidsafe/vault/tests/vault_network.h"

#include <algorithm>
#include <string>

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class VaultNetworkTest : public VaultNetwork  {
 public:
  VaultNetworkTest() {}
};

TEST_F(VaultNetworkTest, FUNC_BasicSetup) {
}

TEST_F(VaultNetworkTest, FUNC_VaultJoins) {
  LOG(kVerbose) << "Adding a vault";
  EXPECT_TRUE(Add());
}

TEST_F(VaultNetworkTest, FUNC_ClientJoins) {
  EXPECT_TRUE(AddClient(false));
}

TEST_F(VaultNetworkTest, FUNC_PmidRegisteringClientJoins) {
  EXPECT_TRUE(AddClient(true));
}

TEST_F(VaultNetworkTest, FUNC_MultipleClientsJoin) {
  for (int index(0); index < 5; ++index)
    EXPECT_TRUE(AddClient(false));
}

TEST_F(VaultNetworkTest, FUNC_PutGetDelete) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData data(NonEmptyString(RandomString(1024)));
  clients_[0]->nfs_->Put(data);
  Sleep(std::chrono::seconds(2));

  auto future(clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5)));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }

  clients_[0]->nfs_->Delete<ImmutableData::Name>(data.name());
  Sleep(std::chrono::seconds(2));

  future = clients_[0]->nfs_->Get<ImmutableData::Name>(data.name(), std::chrono::seconds(5));
  try {
    future.get();
    EXPECT_TRUE(false) << "should have failed retreiveing data: "
                       << DebugId(NodeId(data.name()->string()));
  }
  catch (...) {
    LOG(kVerbose) << DebugId(NodeId(data.name()->string())) << " Deleted ";
  }
}

TEST_F(VaultNetworkTest, FUNC_MultiplePuts) {
  EXPECT_TRUE(AddClient(true));
  const size_t kIterations(50);
  std::vector<ImmutableData> chunks;
  for (auto index(kIterations); index > 0; --index)
    chunks.emplace_back(NonEmptyString(RandomString(1024)));

  int index(0);
  for (const auto& chunk : chunks) {
    clients_[0]->nfs_->Put(chunk);
    Sleep(std::chrono::seconds(2));
    LOG(kVerbose) << DebugId(NodeId(chunk.name()->string())) << " stored: " << index++;
  }

  std::vector<boost::future<ImmutableData>> get_futures;
  for (const auto& chunk : chunks) {
    get_futures.emplace_back(clients_[0]->nfs_->Get<ImmutableData::Name>(chunk.name()));
    Sleep(std::chrono::seconds(1));
  }

  for (size_t index(0); index < kIterations; ++index) {
    try {
      auto retrieved(get_futures[index].get());
      EXPECT_EQ(retrieved.data(), chunks[index].data());
    }
    catch (const std::exception& ex) {
      LOG(kError) << "Failed to retrieve chunk: " << DebugId(chunks[index].name())
                  << " because: " << ex.what() << " "  << index;
    }
  }
  LOG(kVerbose) << "Multiple puts is finished successfully";
}

TEST_F(VaultNetworkTest, FUNC_FailingGet) {
  EXPECT_TRUE(AddClient(true));
  LOG(kVerbose) << "Client joins";
  ImmutableData data(NonEmptyString(RandomString(1024)));
  EXPECT_THROW(Get<ImmutableData>(data.name()), maidsafe_error) << "must have failed";
}

TEST_F(VaultNetworkTest, FUNC_PutMultipleCopies) {
  LOG(kVerbose) << "Clients joining";
  EXPECT_TRUE(AddClient(true));
  EXPECT_TRUE(AddClient(true));
  LOG(kVerbose) << "Clients joined";

  ImmutableData data(NonEmptyString(RandomString(1024)));
  boost::future<ImmutableData> future;
  clients_[0]->nfs_->Put(data);
  Sleep(std::chrono::seconds(2));

  clients_[1]->nfs_->Put(data);
  Sleep(std::chrono::seconds(2));

  {
    future = clients_[0]->nfs_->Get<ImmutableData::Name>(data.name());
    try {
      auto retrieved(future.get());
      EXPECT_EQ(retrieved.data(), data.data());
    }
    catch (...) {
      EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
    }
  }

  LOG(kVerbose) << "1st successful put";

  {
    future = clients_[1]->nfs_->Get<ImmutableData::Name>(data.name());
    try {
      auto retrieved(future.get());
      EXPECT_EQ(retrieved.data(), data.data());
    }
    catch (...) {
      EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
    }
  }

  LOG(kVerbose) << "2nd successful put";

  clients_[0]->nfs_->Delete<ImmutableData::Name>(data.name());
  Sleep(std::chrono::seconds(2));

  LOG(kVerbose) << "1st Delete the chunk";

  try {
    auto retrieved(Get<ImmutableData>(data.name()));
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }

  LOG(kVerbose) << "Chunk still exist as expected";

  clients_[1]->nfs_->Delete<ImmutableData::Name>(data.name());
  Sleep(std::chrono::seconds(2));

  LOG(kVerbose) << "2nd Delete the chunk";

  EXPECT_THROW(Get<ImmutableData>(data.name()), maidsafe_error) << "Should have failed to retreive";

  LOG(kVerbose) << "PutMultipleCopies Succeeds";
}

TEST_F(VaultNetworkTest, FUNC_MultipleClientsPut) {
  int clients(10);
  for (int index(0); index < clients; ++index)
    EXPECT_TRUE(AddClient(true));
  LOG(kVerbose) << "Clients joined...";
  const size_t kIterations(50);
  std::vector<ImmutableData> chunks;
  for (auto index(kIterations); index > 0; --index)
    chunks.emplace_back(NonEmptyString(RandomString(1024)));

  for (const auto& chunk : chunks) {
    clients_[RandomInt32() % clients]->nfs_->Put(chunk);
    Sleep(std::chrono::milliseconds(500));
  }

  LOG(kVerbose) << "Chunks are sent to be stored...";
  Sleep(std::chrono::seconds(10));
  LOG(kVerbose) << "After sleep";

  std::vector<boost::future<ImmutableData>> get_futures;
  for (const auto& chunk : chunks)
    get_futures.emplace_back(
        clients_[RandomInt32() % clients]->nfs_->Get<ImmutableData::Name>(chunk.name()));

  for (size_t index(0); index < kIterations; ++index) {
    try {
      auto retrieved(get_futures[index].get());
      EXPECT_EQ(retrieved.data(), chunks[index].data());
    }
    catch (const std::exception& ex) {
      LOG(kError) << "Failed to retrieve chunk: " << DebugId(chunks[index].name())
                  << " because: " << ex.what();
    }
  }
}

TEST_F(VaultNetworkTest, FUNC_UnauthorisedDelete) {
  EXPECT_TRUE(AddClient(true));
  EXPECT_TRUE(AddClient(true));

  ImmutableData chunk(NonEmptyString(RandomString(2^10)));
  clients_.front()->nfs_->Put<ImmutableData>(chunk);
  Sleep(std::chrono::seconds(2));
  EXPECT_NO_THROW(Get<ImmutableData>(chunk.name()));
  LOG(kVerbose) << "Chunk is verified to be in the network";
  clients_.back()->nfs_->Delete(chunk.name());
  Sleep(std::chrono::seconds(2));
  EXPECT_NO_THROW(Get<ImmutableData>(chunk.name())) << "Delete must have failed";
  clients_.front()->nfs_->Delete(chunk.name());
  Sleep(std::chrono::seconds(2));
  EXPECT_THROW(Get<ImmutableData>(chunk.name()), maidsafe_error)  << "Delete must have succeeded";
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

