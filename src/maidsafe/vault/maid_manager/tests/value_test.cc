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

#include <vector>

#include "maidsafe/common/test.h"

#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/value.h"

namespace maidsafe {

namespace vault {

namespace test {


TEST(MaidManagerValueTest, BEH_DefaultConstruct) {
  MaidManager::Value value;
  ASSERT_EQ(0, value.data_stored);
  ASSERT_EQ(std::numeric_limits<uint64_t>().max(), value.space_available);
}

TEST(MaidManagerValueTest, BEH_Construct) {
  uint64_t data_stored(1), space_available(1);
  MaidManager::Value value(data_stored, space_available);
  ASSERT_EQ(data_stored, value.data_stored);
  ASSERT_EQ(space_available, value.space_available);

  ASSERT_NO_THROW(MaidManager::Value value0(value));
  ASSERT_NO_THROW(MaidManager::Value(value));
}

TEST(MaidManagerValueTest, BEH_Equality) {
  uint64_t data_stored(1), space_available(1);
  MaidManager::Value value1(data_stored, space_available), value2(data_stored, space_available),
                     value3(data_stored, space_available + 1);
  
  ASSERT_TRUE(value1 == value2);
  ASSERT_FALSE(value1 == value3);
}

TEST(MaidManagerValueTest, BEH_Swap) {
  uint64_t data1_stored(0), space1_available(100), data2_stored(1), space2_available(99);
  MaidManager::Value value1(data1_stored, space1_available),
                     value2(data2_stored, space2_available);
  
  ASSERT_EQ(data1_stored, value1.data_stored);
  ASSERT_EQ(space1_available, value1.space_available);
  ASSERT_EQ(data2_stored, value2.data_stored);
  ASSERT_EQ(space2_available, value2.space_available);

  ASSERT_NO_THROW(swap(value1, value2));

  ASSERT_EQ(data2_stored, value1.data_stored);
  ASSERT_EQ(space2_available, value1.space_available);
  ASSERT_EQ(data1_stored, value2.data_stored);
  ASSERT_EQ(space1_available, value2.space_available);
}

TEST(MaidManagerValueTest, BEH_SerialiseParse) {
  uint64_t data_stored(0), space_available(100);
  MaidManager::Value value1(data_stored, space_available), value2;
  ASSERT_EQ(data_stored, value1.data_stored);
  ASSERT_EQ(space_available, value1.space_available);
  std::string serialised_value;
  ASSERT_NO_THROW(serialised_value = value1.Serialise());
  ASSERT_NO_THROW(value2 = MaidManager::Value(serialised_value));
  ASSERT_EQ(value1.data_stored, value2.data_stored);
  ASSERT_EQ(value1.space_available, value2.space_available);
}

TEST(MaidManagerValueTest, BEH_AllowPut) {
  uint64_t data_stored(0), space_available(100);
  MaidManager::Value value(data_stored, space_available);
  ASSERT_EQ(data_stored, value.data_stored);
  ASSERT_EQ(space_available, value.space_available);

  passport::Anpmid anpmid;
  passport::Pmid pmid(anpmid);
  passport::PublicAnpmid public_anpmid(anpmid);
  passport::PublicPmid public_pmid(pmid);

  ASSERT_EQ(MaidManagerValue::Status::kOk, value.AllowPut(public_anpmid));
  ASSERT_EQ(MaidManagerValue::Status::kOk, value.AllowPut(public_pmid));

  NonEmptyString content1(RandomString(2)), content2(RandomString(4)),
                 content3(RandomString(101));
  ImmutableData data1(content1), data2(content2), data3(content3);

  ASSERT_EQ(MaidManagerValue::Status::kOk, value.AllowPut(data1));
  ASSERT_EQ(MaidManagerValue::Status::kLowSpace, value.AllowPut(data2));
  ASSERT_EQ(MaidManagerValue::Status::kNoSpace, value.AllowPut(data3));
}

TEST(MaidManagerValueTest, BEH_PutDelete) {
  uint64_t data_stored(0), space_available(100);
  const uint64_t kSize(100);
  MaidManager::Value value(data_stored, space_available);
  ASSERT_EQ(data_stored, value.data_stored);
  ASSERT_EQ(space_available, value.space_available);

  value.PutData(kSize);
  ASSERT_EQ(space_available, value.data_stored);
  ASSERT_EQ(data_stored, value.space_available);
  value.DeleteData(kSize);
  ASSERT_EQ(data_stored, value.data_stored);
  ASSERT_EQ(space_available, value.space_available);
}

TEST(MaidManagerValueTest, BEH_Resolve) {
  uint64_t data1_stored(49), space1_available(99),
           data2_stored(50), space2_available(100),
           data3_stored(51), space3_available(101);

  MaidManager::Value value1(data1_stored, space1_available);
  ASSERT_EQ(data1_stored, value1.data_stored);
  ASSERT_EQ(space1_available, value1.space_available);

  MaidManager::Value value2(data2_stored, space2_available);
  ASSERT_EQ(data2_stored, value2.data_stored);
  ASSERT_EQ(space2_available, value2.space_available);

  MaidManager::Value value3(data3_stored, space3_available);
  ASSERT_EQ(data3_stored, value3.data_stored);
  ASSERT_EQ(space3_available, value3.space_available);

  MaidManager::Value value;
  ASSERT_EQ(0, value.data_stored);
  ASSERT_EQ(std::numeric_limits<uint64_t>().max(), value.space_available);

  std::vector<MaidManager::Value> values;
  ASSERT_NO_THROW(values.push_back(value1));
  
  ASSERT_THROW(value.Resolve(values), maidsafe_error);

  ASSERT_NO_THROW(values.push_back(value2));
  ASSERT_NO_THROW(values.push_back(value3));

  ASSERT_NO_THROW(value = value.Resolve(values));
  ASSERT_EQ(data2_stored, value.data_stored);
  ASSERT_EQ(space2_available, value.space_available);
}


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
