/*******************************************************************************
*  Copyright 2012 maidsafe.net limited                                        *
*                                                                             *
*  The following source code is property of maidsafe.net limited and is not   *
*  meant for external use.  The use of this code is governed by the licence   *
*  file licence.txt found in the root of this directory and also on           *
*  www.maidsafe.net.                                                          *
*                                                                             *
*  You are not free to copy, amend or otherwise use this source code without  *
*  the explicit written permission of the board of directors of maidsafe.net. *
******************************************************************************/

#include "maidsafe/vault/maid_account_handler.h"

#include <vector>

#include "maidsafe/common/test.h"

namespace maidsafe {

namespace vault {

namespace test {

class MaidAccountHandlerTest : public testing::Test {
 public:
  MaidAccountHandlerTest()
      : vault_root_directory_(maidsafe::test::CreateTestPath("MaidSafe_Test_MaidAccountHandler")),
        maid_account_handler_(*vault_root_directory_) {}

  ~MaidAccountHandlerTest() {}

  MaidName GenerateMaidName() {
    return MaidName(Identity(RandomAlphaNumericString(64)));
  }

  std::vector<MaidAccount> GetMaidAccounts() {
    return maid_account_handler_.maid_accounts_;
  }

  std::vector<PmidTotals> GetPmids(const MaidName& account_name) {
    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
    return itr->pmid_totals_;
  }

  void AddAccount(const MaidAccount& account) {
    std::vector<MaidName> accounts;
    int total_accounts_pre_add = (maid_account_handler_.GetAccountNames()).size();

    EXPECT_TRUE(maid_account_handler_.AddAccount(account));
    accounts = maid_account_handler_.GetAccountNames();
    EXPECT_EQ(total_accounts_pre_add + 1, accounts.size());
  }

 protected:
  maidsafe::test::TestPath vault_root_directory_;
  MaidAccountHandler maid_account_handler_;
};


TEST_F(MaidAccountHandlerTest, BEH_AddAccount) {
  std::vector<MaidName> account_names;
  MaidName account_name(GenerateMaidName());
  MaidAccount new_account(account_name, *vault_root_directory_);

  AddAccount(new_account);

  EXPECT_FALSE(maid_account_handler_.AddAccount(new_account));
  account_names = maid_account_handler_.GetAccountNames();
  EXPECT_EQ(1, account_names.size());
}

TEST_F(MaidAccountHandlerTest, BEH_DeleteAccount) {
  std::vector<MaidName> account_names;
  MaidName account_name(GenerateMaidName());
  MaidAccount new_account(account_name, *vault_root_directory_);

  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));

  AddAccount(new_account);

  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));
  account_names = maid_account_handler_.GetAccountNames();
  EXPECT_EQ(0, account_names.size());
}

TEST_F(MaidAccountHandlerTest, BEH_RegisterPmid) {
  MaidName account_name(GenerateMaidName());
  MaidAccount new_account(account_name, *vault_root_directory_);
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  nfs::PmidRegistration registration(maid, pmid, false);
  nfs::PmidRegistration::serialised_type serialised_registration(registration.Serialise());

  EXPECT_THROW(maid_account_handler_.RegisterPmid(account_name, serialised_registration),
               std::exception);
  AddAccount(new_account);
  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, serialised_registration));

  std::vector<PmidTotals> pmids = GetPmids(account_name);
  EXPECT_NE(pmids.end(), std::find_if(pmids.begin(),
                                      pmids.end(),
                                      [&registration](const PmidTotals record) {
                                        return registration.pmid_name() ==
                                            record.pmid_record.pmid_name;
                                      }));
}

TEST_F(MaidAccountHandlerTest, BEH_UnregisterPmid) {
  MaidName account_name(GenerateMaidName());
  MaidAccount account(account_name, *vault_root_directory_);
  PmidName pmid_name(Identity(RandomAlphaNumericString(64)));

  EXPECT_THROW(maid_account_handler_.UnregisterPmid(account_name, pmid_name), std::exception);

  AddAccount(account);
  EXPECT_NO_THROW(maid_account_handler_.UnregisterPmid(account_name, pmid_name));

  std::vector<PmidTotals> pmids = GetPmids(account_name);
  EXPECT_EQ(pmids.end(), std::find_if(pmids.begin(),
                                      pmids.end(),
                                      [&pmid_name](const PmidTotals record) {
                                        return pmid_name == record.pmid_record.pmid_name;
                                      }));
}

TEST_F(MaidAccountHandlerTest, BEH_UpdatePmidTotals) {
  MaidName account_name(GenerateMaidName());
  MaidAccount account(account_name, *vault_root_directory_);
  PmidTotals pmid_totals;
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  nfs::PmidRegistration registration(maid, pmid, false);
  nfs::PmidRegistration::serialised_type serialised_registration(registration.Serialise());

  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), std::exception);
  AddAccount(account);
  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), std::exception);
  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, serialised_registration));
  EXPECT_NO_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals));
}

template <typename Data>
class MaidAccountHandlerTypedTest : public MaidAccountHandlerTest {
 public:
  MaidAccountHandlerTypedTest()
    : MaidAccountHandlerTest() {}

 protected:
  void PutData(const MaidName& account_name,
               const typename Data::name_type& data_name,
               int32_t size,
               int32_t replication_count) {
    this->maid_account_handler_.template PutData<Data>(account_name,
                                                       data_name,
                                                       size,
                                                       replication_count);
  }

  void DeleteData(const MaidName& account_name, const typename Data::name_type& data_name) {
    this->maid_account_handler_.template DeleteData<Data>(account_name, data_name);
  }

  void UpdateReplicationCount(const MaidName& account_name,
                              const typename Data::name_type& data_name,
                              int32_t new_replication_count) {
    this->maid_account_handler_.template UpdateReplicationCount<Data>(account_name,
                                                                      data_name,
                                                                      new_replication_count);
  }
};

TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest);

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_PutData) {
  typename TypeParam::name_type data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
  MaidName account_name(this->GenerateMaidName());
  int32_t size(1000), replication_count(4);
  MaidAccount account(account_name, *(this->vault_root_directory_));

  EXPECT_THROW(this->PutData(account_name, data_name, size, replication_count), std::exception);
  this->AddAccount(account);
  EXPECT_NO_THROW(this->PutData(account_name, data_name, size, replication_count));
}

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_DeleteData) {
  typename TypeParam::name_type data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
  MaidName account_name(this->GenerateMaidName());
  MaidAccount account(account_name, *(this->vault_root_directory_));

  EXPECT_THROW(this->DeleteData(account_name, data_name), std::exception);
  this->AddAccount(account);
  EXPECT_NO_THROW(this->DeleteData(account_name, data_name));
}

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_UpdateReplicationCount) {
  typename TypeParam::name_type data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
  MaidName account_name(this->GenerateMaidName());
  int32_t new_replication_count(3);
  MaidAccount account(account_name, *(this->vault_root_directory_));

  EXPECT_THROW(this->UpdateReplicationCount(account_name, data_name, new_replication_count),
               std::exception);
  this->AddAccount(account);
  EXPECT_NO_THROW(this->UpdateReplicationCount(account_name, data_name, new_replication_count));
}

REGISTER_TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest,
                           BEH_PutData,
                           BEH_DeleteData,
                           BEH_UpdateReplicationCount);

typedef testing::Types<passport::PublicAnmid,
                       passport::PublicAnsmid,
                       passport::PublicAntmid,
                       passport::PublicAnmaid,
                       passport::PublicMaid,
                       passport::PublicPmid,
                       passport::Mid,
                       passport::Smid,
                       passport::Tmid,
                       passport::PublicAnmpid,
                       passport::PublicMpid,
                       ImmutableData,
                       MutableData> AllTypes;

INSTANTIATE_TYPED_TEST_CASE_P(All, MaidAccountHandlerTypedTest, AllTypes);


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
