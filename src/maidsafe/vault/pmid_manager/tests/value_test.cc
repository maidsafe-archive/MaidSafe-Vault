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

#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace test {

class PmidManagerValueTest : public testing::Test {
 public:
  PmidManagerValueTest() {}
};

TEST_F(PmidManagerValueTest, BEH_ParseFromString) {
  EXPECT_ANY_THROW(PmidManagerValue(RandomString(64)));
  PmidManagerValue empty_value;
  EXPECT_NO_THROW(PmidManagerValue(empty_value.Serialise()));
}

TEST_F(PmidManagerValueTest, BEH_Assignment) {
  EXPECT_ANY_THROW(PmidManagerValue(RandomString(64)));
  PmidManagerValue value(kTestChunkSize, 0, kTestChunkSize);
  PmidManagerValue empty_value;
  empty_value = value;
  EXPECT_EQ(value, empty_value);
}

TEST_F(PmidManagerValueTest, BEH_DeleteData) {
  PmidManagerValue empty_value;
  EXPECT_ANY_THROW(empty_value.DeleteData(kTestChunkSize));
  PmidManagerValue value(kTestChunkSize, 0, kTestChunkSize);
  EXPECT_NO_THROW(value.DeleteData(kTestChunkSize / 2));
  EXPECT_EQ(kTestChunkSize / 2, value.stored_total_size);
  EXPECT_EQ(0, value.lost_total_size);
  EXPECT_EQ(kTestChunkSize, value.offered_space);
}

TEST_F(PmidManagerValueTest, BEH_LostData) {
  PmidManagerValue empty_value;
  EXPECT_ANY_THROW(empty_value.HandleLostData(kTestChunkSize));
  PmidManagerValue value(kTestChunkSize, 0, kTestChunkSize);
  EXPECT_NO_THROW(value.HandleLostData(kTestChunkSize));
  EXPECT_EQ(0, value.stored_total_size);
  EXPECT_EQ(kTestChunkSize, value.lost_total_size);
  EXPECT_EQ(kTestChunkSize, value.offered_space);
}

TEST_F(PmidManagerValueTest, BEH_FailureData) {
  PmidManagerValue empty_value;
  EXPECT_ANY_THROW(empty_value.HandleFailure(kTestChunkSize));
  PmidManagerValue value(kTestChunkSize, kTestChunkSize, kTestChunkSize);
  EXPECT_NO_THROW(value.HandleFailure(kTestChunkSize));
  EXPECT_EQ(0, value.stored_total_size);
  EXPECT_EQ(kTestChunkSize * 2, value.lost_total_size);
  EXPECT_EQ(kTestChunkSize, value.offered_space);
}

TEST_F(PmidManagerValueTest, BEH_OfferedSpace) {
  PmidManagerValue empty_value;
  EXPECT_NO_THROW(empty_value.SetAvailableSize(kTestChunkSize));
  EXPECT_EQ(0, empty_value.stored_total_size);
  EXPECT_EQ(0, empty_value.lost_total_size);
  EXPECT_EQ(kTestChunkSize, empty_value.offered_space);
  EXPECT_NO_THROW(empty_value.SetAvailableSize(0));
  EXPECT_EQ(0, empty_value.offered_space);
}

TEST_F(PmidManagerValueTest, BEH_GroupStatus) {
  PmidManagerValue empty_value;
  EXPECT_EQ(detail::GroupDbMetaDataStatus::kGroupEmpty, empty_value.GroupStatus());
  EXPECT_NO_THROW(empty_value.SetAvailableSize(kTestChunkSize));
  EXPECT_EQ(detail::GroupDbMetaDataStatus::kGroupNonEmpty, empty_value.GroupStatus());
}

TEST_F(PmidManagerValueTest, BEH_Print) {
  PmidManagerValue empty_value;
  EXPECT_NO_THROW(empty_value.Print());
  PmidManagerValue value(kTestChunkSize, 0, kTestChunkSize);
  EXPECT_NO_THROW(value.Print());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
