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
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/maid_manager/account.h"

namespace maidsafe {

namespace vault {

namespace test {


TEST(MaidManagerAccountTest, BEH_Construct) {
  {
    MaidManagerAccount::AccountName name(Identity(RandomAlphaNumericString(64)));
    uint64_t data_stored(1), space_available(1);
    MaidManagerAccount account(name, data_stored, space_available);

    ASSERT_EQ(name, account.name());
    ASSERT_EQ(data_stored, account.data_stored());
    ASSERT_EQ(space_available, account.space_available());

    ASSERT_NO_THROW(MaidManagerAccount account_copy(account));
    ASSERT_NO_THROW(MaidManagerAccount(std::move(account)));
  }
  {
    MaidManagerAccount::AccountName name1(Identity(RandomAlphaNumericString(64)));
    uint64_t data1_stored(1), space1_available(1);
    MaidManagerAccount account1(name1, data1_stored, space1_available);

    ASSERT_EQ(name1, account1.name());
    ASSERT_EQ(data1_stored, account1.data_stored());
    ASSERT_EQ(space1_available, account1.space_available());

    MaidManagerAccount::AccountName name2(Identity(RandomAlphaNumericString(64)));
    uint64_t data2_stored(2), space2_available(2);
    MaidManagerAccount account2(name2, data2_stored, space2_available);

    ASSERT_EQ(name2, account2.name());
    ASSERT_EQ(data2_stored, account2.data_stored());
    ASSERT_EQ(space2_available, account2.space_available());

    ASSERT_NO_THROW(account2 = account1);
    ASSERT_NE(name2, account2.name());
    ASSERT_NE(data2_stored, account2.data_stored());
    ASSERT_NE(space2_available, account2.space_available());
    ASSERT_EQ(account2.name(), account1.name());
    ASSERT_EQ(account2.data_stored(), account1.data_stored());
    ASSERT_EQ(account2.space_available(), account1.space_available());
  }
}

TEST(MaidManagerAccountTest, BEH_SerialiseParse) {
  MaidManagerAccount::AccountName name(Identity(RandomAlphaNumericString(64)));
  uint64_t data_stored(0), space_available(100);
  MaidManagerAccount account1(name, data_stored, space_available);

  ASSERT_EQ(name, account1.name());
  ASSERT_EQ(data_stored, account1.data_stored());
  ASSERT_EQ(space_available, account1.space_available());

  std::string serialised_account;
  ASSERT_NO_THROW(serialised_account = account1.serialise());
  MaidManagerAccount account2(serialised_account);

  ASSERT_EQ(name, account2.name());
  ASSERT_EQ(data_stored, account2.data_stored());
  ASSERT_EQ(space_available, account2.space_available());
}

TEST(MaidManagerAccountTest, BEH_EqualityInequality) {
  MaidManagerAccount::AccountName name(Identity(RandomAlphaNumericString(64)));
  uint64_t data_stored(1), space_available(1);
  MaidManagerAccount account1(name, data_stored, space_available),
                     account2(name, data_stored, space_available),
                     account3(name, data_stored, space_available + 1);

  ASSERT_TRUE(account1 == account2);
  ASSERT_FALSE(account1 == account3);
  ASSERT_FALSE(account1 != account2);
  ASSERT_TRUE(account1 != account3);
}

TEST(MaidManagerAccountTest, BEH_Ordering) {
  MaidManagerAccount::AccountName name1(Identity("A" + RandomAlphaNumericString(63))),
                                  name2(Identity("B" + RandomAlphaNumericString(63)));
  uint64_t data_stored(1), space_available(1);
  MaidManagerAccount account1(name1, data_stored, space_available),
                     account2(name2, data_stored, space_available),
                     account3(name1, data_stored, space_available);

  ASSERT_TRUE(account1 <= account3);
  ASSERT_TRUE(account1 >= account3);
  ASSERT_FALSE(account1 < account3);
  ASSERT_FALSE(account1 > account3);

  ASSERT_TRUE(account1 <= account2);
  ASSERT_FALSE(account1 >= account2);
  ASSERT_TRUE(account1 < account2);
  ASSERT_FALSE(account1 > account2);
}

TEST(MaidManagerAccountTest, BEH_AllowPut) {
  MaidManagerAccount::AccountName name(Identity(RandomAlphaNumericString(64)));
  uint64_t data_stored(0), space_available(100);
  MaidManagerAccount account(name, data_stored, space_available);

  ASSERT_EQ(name, account.name());
  ASSERT_EQ(data_stored, account.data_stored());
  ASSERT_EQ(space_available, account.space_available());

  passport::Anpmid anpmid;
  passport::Pmid pmid(anpmid);
  passport::PublicAnpmid public_anpmid(anpmid);
  passport::PublicPmid public_pmid(pmid);

  ASSERT_EQ(MaidManagerAccount::Status::kOk, account.AllowPut(public_anpmid));
  ASSERT_EQ(MaidManagerAccount::Status::kOk, account.AllowPut(public_pmid));

  NonEmptyString content1(RandomString(2)), content2(RandomString(4)),
                 content3(RandomString(101));
  ImmutableData data1(content1), data2(content2), data3(content3);

  ASSERT_EQ(MaidManagerAccount::Status::kOk, account.AllowPut(data1));
  ASSERT_EQ(MaidManagerAccount::Status::kLowSpace, account.AllowPut(data2));
  ASSERT_EQ(MaidManagerAccount::Status::kNoSpace, account.AllowPut(data3));
}

TEST(MaidManagerAccountTest, BEH_PutDelete) {
  MaidManagerAccount::AccountName name(Identity(RandomAlphaNumericString(64)));
  uint64_t data_stored(0), space_available(100);
  const uint64_t kSize(100);
  MaidManagerAccount account(name, data_stored, space_available);

  ASSERT_EQ(name, account.name());
  ASSERT_EQ(data_stored, account.data_stored());
  ASSERT_EQ(space_available, account.space_available());

  account.PutData(kSize);
  ASSERT_EQ(space_available, account.data_stored());
  ASSERT_EQ(data_stored, account.space_available());
  account.DeleteData(kSize);
  ASSERT_EQ(data_stored, account.data_stored());
  ASSERT_EQ(space_available, account.space_available());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
