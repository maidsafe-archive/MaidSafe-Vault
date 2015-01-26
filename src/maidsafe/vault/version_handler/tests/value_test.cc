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

#include "maidsafe/vault/version_handler/value.h"
#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class VersionHandlerValueTest : public testing::Test {
 public:
  VersionHandlerValueTest() {}
};

TEST_F(VersionHandlerValueTest, BEH_Constructor) {
  std::shared_ptr<VersionHandlerValue> value_ptr;
  EXPECT_ANY_THROW(value_ptr.reset(new VersionHandlerValue(RandomString(64))));
}

TEST_F(VersionHandlerValueTest, BEH_Swap) {
  VersionHandlerValue value_1(10, 1);
  VersionHandlerValue value_2(3, 4);
  EXPECT_NO_THROW(std::swap(value_1, value_2));
}

TEST_F(VersionHandlerValueTest, BEH_Get) {
  VersionHandlerValue value(10, 1);
  StructuredDataVersions::VersionName version_name(
      0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  value.Put(StructuredDataVersions::VersionName(), version_name);
  auto result(value.Get());
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(version_name, result.front());
}

TEST_F(VersionHandlerValueTest, BEH_Print) {
  VersionHandlerValue value(10, 1);
  StructuredDataVersions::VersionName version_name(
      0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  value.Put(StructuredDataVersions::VersionName(), version_name);
  EXPECT_NO_THROW(value.Print());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
