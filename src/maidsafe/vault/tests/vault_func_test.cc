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

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {

class VaultTest : public testing::Test {
 public:
  VaultTest() : env_(VaultEnvironment::g_environment()) {}

  std::vector<VaultNetwork::ClientPtr>& GetClients() {
    return env_->clients_;
  }

 protected:
  std::shared_ptr<VaultNetwork> env_;
};

TEST_F(VaultTest, FUNC_PutGet) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  LOG(kVerbose) << "Before put";
  try {
    EXPECT_NO_THROW(GetClients().front()->Put(data));
    LOG(kVerbose) << "After put";
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to put: " << DebugId(NodeId(data.name()->string()));
  }

  auto future(GetClients().front()->Get<ImmutableData::Name>(data.name()));
  try {
    auto retrieved(future.get());
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }
}

TEST_F(VaultTest, FUNC_MultiplePuts) {
  routing::Parameters::caching = true;
  const size_t kIterations(100);
  std::vector<ImmutableData> chunks;
  for (auto index(kIterations); index > 0; --index)
    chunks.emplace_back(NonEmptyString(RandomString(1024)));

  int index(0);
  for (const auto& chunk : chunks) {
    EXPECT_NO_THROW(GetClients().front()->Put(chunk))
                        << "Store failure " << DebugId(NodeId(chunk.name()->string()));
    LOG(kVerbose) << DebugId(NodeId(chunk.name()->string())) << " stored: " << index++;
  }

  std::vector<boost::future<ImmutableData>> get_futures;
  for (const auto& chunk : chunks) {
    get_futures.emplace_back(
        GetClients().front()->Get<ImmutableData::Name>(chunk.name(),
                                                     std::chrono::seconds(kIterations * 2)));
  }

  for (size_t index(0); index < kIterations; ++index) {
    try {
      auto retrieved(get_futures[index].get());
      EXPECT_EQ(retrieved.data(), chunks[index].data());
      LOG(kVerbose) << "Cache enabled Retrieved: " << index;
    }
    catch (const std::exception& ex) {
      EXPECT_TRUE(false) << "Failed to retrieve chunk: " << DebugId(chunks[index].name())
                         << " because: " << boost::diagnostic_information(ex) << " "  << index;
    }
  }

  routing::Parameters::caching = false;
  // allow time for any pending put
  Sleep(std::chrono::seconds(10));
  std::vector<boost::future<ImmutableData>> no_cache_get_futures;
  for (const auto& chunk : chunks) {
    no_cache_get_futures.emplace_back(
        GetClients().front()->Get<ImmutableData::Name>(chunk.name(),
                                                           std::chrono::seconds(kIterations)));
  }

  for (size_t index(0); index < kIterations; ++index) {
    try {
      auto retrieved(no_cache_get_futures[index].get());
      EXPECT_EQ(retrieved.data(), chunks[index].data());
      LOG(kVerbose) << "Cache disabled Retrieved: " << index;
    }
    catch (const std::exception& ex) {
      EXPECT_TRUE(false) << "Failed to retrieve chunk: " << DebugId(chunks[index].name())
                         << " because: " << boost::diagnostic_information(ex) << " "  << index;
    }
  }
  LOG(kVerbose) << "Multiple puts is finished successfully";
}

TEST_F(VaultTest, FUNC_FailingGet) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  EXPECT_THROW(env_->Get<ImmutableData>(data.name()), std::exception) << "must have failed";
}

// The test below is disbaled as its proper operation assumes a delete funcion is in place
TEST_F(VaultTest, DISABLED_FUNC_PutMultipleCopies) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  boost::future<ImmutableData> future;
  GetClients().front()->Put(data);
  Sleep(std::chrono::seconds(2));

  GetClients().back()->Put(data);
  Sleep(std::chrono::seconds(2));

  {
    future = GetClients().front()->Get<ImmutableData::Name>(data.name());
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
    future = GetClients().back()->Get<ImmutableData::Name>(data.name());
    try {
      auto retrieved(future.get());
      EXPECT_EQ(retrieved.data(), data.data());
    }
    catch (...) {
      EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
    }
  }

  LOG(kVerbose) << "2nd successful put";

  GetClients().front()->Delete<ImmutableData::Name>(data.name());
  Sleep(std::chrono::seconds(2));

  LOG(kVerbose) << "1st Delete the chunk";

  try {
    auto retrieved(env_->Get<ImmutableData>(data.name()));
    EXPECT_EQ(retrieved.data(), data.data());
  }
  catch (...) {
    EXPECT_TRUE(false) << "Failed to retrieve: " << DebugId(NodeId(data.name()->string()));
  }

  LOG(kVerbose) << "Chunk still exist as expected";

  GetClients().back()->Delete<ImmutableData::Name>(data.name());
  Sleep(std::chrono::seconds(2));

  LOG(kVerbose) << "2nd Delete the chunk";

  routing::Parameters::caching = false;

  EXPECT_THROW(env_->Get<ImmutableData>(data.name()), std::exception)
                   << "Should have failed to retreive";

  LOG(kVerbose) << "PutMultipleCopies Succeeds";
  routing::Parameters::caching = true;
}

TEST_F(VaultTest, FUNC_MultipleClientsPut) {
  const size_t kIterations(10);
  std::vector<ImmutableData> chunks;
  for (auto index(kIterations); index > 0; --index)
    chunks.emplace_back(NonEmptyString(RandomString(1024)));

  for (const auto& chunk : chunks) {
    LOG(kVerbose) << "Storing: " << DebugId(chunk.name());
    EXPECT_NO_THROW(GetClients().at(RandomInt32() % kClientsSize)->Put(chunk));
  }

  LOG(kVerbose) << "Chunks are sent to be stored...";

  std::vector<boost::future<ImmutableData>> get_futures;
  for (const auto& chunk : chunks)
    get_futures.emplace_back(
        GetClients().at(RandomInt32() % kClientsSize)->Get<ImmutableData::Name>(chunk.name()));

  for (size_t index(0); index < kIterations; ++index) {
    try {
      auto retrieved(get_futures[index].get());
      EXPECT_EQ(retrieved.data(), chunks[index].data());
    }
    catch (const std::exception& ex) {
      LOG(kError) << "Failed to retrieve chunk: " << DebugId(chunks[index].name())
                  << " because: " << boost::diagnostic_information(ex);
    }
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
