/*  Copyright 2015 MaidSafe.net limited

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
#include "maidsafe/passport/passport.h"

#include "maidsafe/vault/pmid_node/handler.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class PmidNodeHandlerTest : public testing::Test {
 public:
  PmidNodeHandlerTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_ / RandomAlphaNumericString(8)) {
    boost::filesystem::create_directory(vault_root_dir_);
  }

 protected:
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
};

TEST_F(PmidNodeHandlerTest, BEH_GetDiskPath) {
  PmidNodeHandler handler(vault_root_dir_, DiskUsage(1000));
  boost::filesystem::path handler_disk_path(vault_root_dir_ / "pmid_node" / "permanent");
  EXPECT_EQ(handler_disk_path, handler.GetDiskPath());
}

TEST_F(PmidNodeHandlerTest, BEH_AvailableSpace) {
  PmidNodeHandler handler(vault_root_dir_, DiskUsage(1000));
  boost::filesystem::path handler_disk_path(vault_root_dir_ / "pmid_node" / "permanent");
  auto available_space(boost::filesystem::space(handler_disk_path));
  EXPECT_LE(available_space.available, handler.AvailableSpace());
}

TEST_F(PmidNodeHandlerTest, BEH_GetAllDataNames) {
  boost::filesystem::path vault_root_dir(*kTestRoot_ / RandomAlphaNumericString(8));
  boost::filesystem::create_directory(vault_root_dir);
  PmidNodeHandler handler(vault_root_dir, DiskUsage(10000000));
  {
    auto result(handler.GetAllDataNames());
    EXPECT_TRUE(result.empty());
  }
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  handler.Put(data);
  {
    auto result(handler.GetAllDataNames());
    EXPECT_EQ(1, result.size());
  }
  handler.Delete(data.name());
  {
    auto result(handler.GetAllDataNames());
    EXPECT_TRUE(result.empty());
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
