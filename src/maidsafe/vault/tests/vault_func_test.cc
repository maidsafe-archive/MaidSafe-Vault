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

  std::vector<VaultNetwork::ClientPtr>& GetClients() { return env_->clients_; }

 protected:
  std::shared_ptr<VaultNetwork> env_;
};

TEST_F(VaultTest, FUNC_KeepsJoin) {
  for (size_t i(0); i < 40; ++i) {
    LOG(kVerbose) << "Joining vault number " << i;
    EXPECT_TRUE(env_->AddVault()) << " Vault " << i << " failed in joining network";
    Sleep(std::chrono::seconds(2));
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
