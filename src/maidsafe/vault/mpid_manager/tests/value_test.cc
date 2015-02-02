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

#include "maidsafe/passport/types.h"

#include "maidsafe/vault/mpid_manager/value.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

TEST(MpidManagerValueTest, BEH_Constructors) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  {
    auto data_copy = data;
    MpidManagerValue value(std::move(data_copy));  // rvalue ImmutableData input
    EXPECT_NE(data.data(), data_copy.data());
    EXPECT_EQ(value.data.data(), data.data());
  }

  {
    MpidManagerValue value(data);                  // lvalue ImmutableData input
    EXPECT_EQ(value.data.data(), data.data());
  }

  {
    MpidManagerValue value(data);
    auto value_copy(value);                        // copy constructor
    EXPECT_EQ(value_copy, value);                  // == operator
  }

  {
    MpidManagerValue value(data);
    auto value_copy(std::move(value));             // move constructor
    EXPECT_NE(value_copy, value);
    EXPECT_EQ(value_copy.data.data(), data.data());
  }

  {
    MpidManagerValue value(data);
    auto serialised_value(value.Serialise());      // serialised string constructor
    MpidManagerValue copy_value(serialised_value);
    EXPECT_EQ(value, copy_value);
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
