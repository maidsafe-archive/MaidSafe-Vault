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



#include <algorithm>

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/tests/vault_network.h"

namespace maidsafe {

namespace vault {

namespace test {

class VersionHandlerTest : public VaultNetwork  {
 public:
  VersionHandlerTest() {}
};

TEST_F(VersionHandlerTest, FUNC_PutGet) {
  EXPECT_TRUE(AddClient(true));
  ImmutableData chunk(NonEmptyString(RandomAlphaNumericString(1024)));
  StructuredDataVersions::VersionName v_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  EXPECT_NO_THROW(clients_.front()->nfs_->PutVersion(
                      chunk.name(), StructuredDataVersions::VersionName(), v_aaa));
  Sleep(std::chrono::seconds(2));
  try {
    auto future(clients_.front()->nfs_->GetVersions(chunk.name()));
    auto versions(future.get());
    EXPECT_EQ(versions.front().id, v_aaa.id);
  } catch(const maidsafe_error& error) {
    EXPECT_TRUE(false) << "Failed to retrieve version: " << error.what();
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

