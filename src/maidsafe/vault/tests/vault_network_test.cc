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

class VaultNetworkTest : public VaultNetwork, public testing::Test {
 public:
  VaultNetworkTest() : VaultNetwork() {}

  virtual void SetUp() {
    VaultNetwork::SetUp();
  }
  virtual void TearDown() {
    VaultNetwork::TearDown();
  }

  virtual ~VaultNetworkTest() {}
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

TEST_F(VaultNetworkTest, FUNC_UnauthorisedDelete) {
  EXPECT_TRUE(AddClient(true));
  EXPECT_TRUE(AddClient(true));

  routing::Parameters::caching = false;
  ImmutableData chunk(NonEmptyString(RandomString(2^10)));
  EXPECT_NO_THROW(clients_.front()->nfs_->Put<ImmutableData>(chunk)) << "should have succeeded";
  EXPECT_NO_THROW(Get<ImmutableData>(chunk.name()));
  LOG(kVerbose) << "Chunk is verified to be in the network";
  clients_.back()->nfs_->Delete(chunk.name());
  Sleep(std::chrono::seconds(3));
  EXPECT_NO_THROW(Get<ImmutableData>(chunk.name())) << "Delete must have failed";
  clients_.front()->nfs_->Delete(chunk.name());
  Sleep(std::chrono::seconds(3));
  EXPECT_THROW(Get<ImmutableData>(chunk.name()), std::exception)  << "Delete must have succeeded";
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
