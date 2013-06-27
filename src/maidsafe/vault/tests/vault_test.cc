/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/vault.h"

#include <functional>
#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"


namespace maidsafe {

namespace vault {

namespace test {

class VaultTest : public testing::Test {
 public:
  VaultTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_directory_(*kTestRoot_ / RandomAlphaNumericString(8)),
        pmid_(MakePmid()),
        on_new_bootstrap_endpoint_([](boost::asio::ip::udp::endpoint /*endpoint*/) {}),
        vault_() {
    boost::filesystem::create_directory(vault_root_directory_);
  }

 protected:
  passport::Maid MakeMaid() {
    passport::Anmaid anmaid;
    return passport::Maid(anmaid);
  }

  passport::Pmid MakePmid() {
    return passport::Pmid(MakeMaid());
  }

  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_directory_;
  passport::Pmid pmid_;
  std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint_;
  std::unique_ptr<Vault> vault_;
};

TEST_F(VaultTest, BEH_Constructor) {
  vault_.reset(new Vault(pmid_, vault_root_directory_, on_new_bootstrap_endpoint_));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
