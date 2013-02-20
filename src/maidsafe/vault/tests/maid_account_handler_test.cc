/******************************************************************************
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

#include "maidsafe/vault/maid_account_holder/maid_account_handler.h"

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

  PmidRecord GeneratePmidRecord(PmidName name,
                                int64_t stored_count,
                                int64_t stored_total_size,
                                int64_t lost_count,
                                int64_t lost_total_size,
                                int64_t claimed_available_size) {
    PmidRecord new_record(name);
    new_record.stored_count = stored_count;
    new_record.stored_total_size = stored_total_size;
    new_record.lost_count = lost_count;
    new_record.lost_total_size = lost_total_size;
    new_record.claimed_available_size = claimed_available_size;
    return new_record;
  }

  std::vector<PmidTotals> GetPmids(const MaidName& account_name) {
    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
    return (*itr)->pmid_totals_;
  }

  void AddAccount(std::unique_ptr<MaidAccount> account) {
    int total_accounts_pre_add = (maid_account_handler_.GetAccountNames()).size();

    EXPECT_TRUE(maid_account_handler_.AddAccount(std::move(account)));
    std::vector<MaidName> account_names(maid_account_handler_.GetAccountNames());
    EXPECT_EQ(total_accounts_pre_add + 1, account_names.size());
  }

  void SetTotalAvailable(MaidName account_name, int64_t total) {
    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
    if (itr != maid_account_handler_.maid_accounts_.end())
      (*itr)->total_claimed_available_size_by_pmids_ = total;
  }

 protected:
  maidsafe::test::TestPath vault_root_directory_;
  MaidAccountHandler maid_account_handler_;
};


TEST_F(MaidAccountHandlerTest, BEH_AddAccount) {
  std::vector<MaidName> account_names;
  MaidName account_name(GenerateMaidName());
  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));
  std::unique_ptr<MaidAccount> duplicate(new MaidAccount(account_name, *vault_root_directory_));

  AddAccount(std::move(new_account));
  account_names = maid_account_handler_.GetAccountNames();
  EXPECT_EQ(1, account_names.size());

  EXPECT_FALSE(maid_account_handler_.AddAccount(std::move(duplicate)));
  account_names = maid_account_handler_.GetAccountNames();
  EXPECT_EQ(1, account_names.size());
}

TEST_F(MaidAccountHandlerTest, BEH_DeleteAccount) {
  std::vector<MaidName> account_names;
  MaidName account_name(GenerateMaidName());
  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));

  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));

  AddAccount(std::move(new_account));

  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));
  account_names = maid_account_handler_.GetAccountNames();
  EXPECT_EQ(0, account_names.size());
}

TEST_F(MaidAccountHandlerTest, BEH_RegisterPmid) {
  MaidName account_name(GenerateMaidName());
  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  nfs::PmidRegistration registration(maid, pmid, false);
  std::vector<PmidTotals> pmids;

  EXPECT_THROW(maid_account_handler_.RegisterPmid(account_name, registration), vault_error);
  AddAccount(std::move(new_account));
  pmids = GetPmids(account_name);
  EXPECT_EQ(0, pmids.size());
  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));

  pmids = GetPmids(account_name);
  EXPECT_EQ(1, pmids.size());
  EXPECT_NE(pmids.end(), std::find_if(pmids.begin(),
                                      pmids.end(),
                                      [&registration](const PmidTotals record) {
                                        return registration.pmid_name() ==
                                            record.pmid_record.pmid_name;
                                      }));

  // same pmid attempted to be registered more than once
  int total_pre_duplicate_register = pmids.size();
  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));
  pmids = GetPmids(account_name);
  EXPECT_EQ(total_pre_duplicate_register, pmids.size());
}

TEST_F(MaidAccountHandlerTest, BEH_UnregisterPmid) {
  MaidName account_name(GenerateMaidName());
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));
  PmidName pmid_name(Identity(RandomAlphaNumericString(64)));

  EXPECT_THROW(maid_account_handler_.UnregisterPmid(account_name, pmid_name), vault_error);

  AddAccount(std::move(account));
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
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));

  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  nfs::PmidRegistration registration(maid, pmid, false);
  nfs::PmidRegistration::serialised_type serialised_registration(registration.Serialise());

  PmidName pmid_name(pmid.name());
  PmidRecord pmid_record(GeneratePmidRecord(pmid_name, 2, 2000, 1, 1000, 15000));
  PmidTotals pmid_totals(serialised_registration, pmid_record);

  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), vault_error);
  AddAccount(std::move(account));
  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), common_error);
  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));

  std::vector<PmidTotals> pmids = GetPmids(account_name);
  EXPECT_EQ(1, pmids.size());
  for (auto& pmidtotals : pmids) {
    EXPECT_EQ(pmid_record.pmid_name, pmidtotals.pmid_record.pmid_name);
    EXPECT_EQ(0, pmidtotals.pmid_record.stored_count);
    EXPECT_EQ(0, pmidtotals.pmid_record.stored_total_size);
    EXPECT_EQ(0, pmidtotals.pmid_record.lost_count);
    EXPECT_EQ(0, pmidtotals.pmid_record.lost_total_size);
  }

  EXPECT_NO_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals));

  pmids = GetPmids(account_name);
  EXPECT_EQ(1, pmids.size());
  for (auto& pmidtotals : pmids) {
    EXPECT_EQ(pmid_record.pmid_name, pmidtotals.pmid_record.pmid_name);
    EXPECT_EQ(2, pmidtotals.pmid_record.stored_count);
    EXPECT_EQ(2000, pmidtotals.pmid_record.stored_total_size);
    EXPECT_EQ(1, pmidtotals.pmid_record.lost_count);
    EXPECT_EQ(1000, pmidtotals.pmid_record.lost_total_size);
  }
}

TEST_F(MaidAccountHandlerTest, BEH_GetAccountNames) {
}

TEST_F(MaidAccountHandlerTest, BEH_GetSerialisedAccount) {
}

TEST_F(MaidAccountHandlerTest, BEH_GetArchiveFileNames) {
}

TEST_F(MaidAccountHandlerTest, BEH_GetArchiveFile) {
}

TEST_F(MaidAccountHandlerTest, BEH_PutArchiveFile) {
}

template <typename Data>
class MaidAccountHandlerTypedTest : public MaidAccountHandlerTest {
 public:
  MaidAccountHandlerTypedTest()
    : MaidAccountHandlerTest() {}

 protected:
  void PutData(const MaidName& account_name,
               const typename Data::name_type& data_name,
               int32_t cost) {
    this->maid_account_handler_.template PutData<Data>(account_name,
                                                       data_name,
                                                       cost);
  }

  void DeleteData(const MaidName& account_name, const typename Data::name_type& data_name) {
    this->maid_account_handler_.template DeleteData<Data>(account_name, data_name);
  }

  void Adjust(const MaidName& account_name,
              const typename Data::name_type& data_name,
              int32_t new_cost)
  {
    this->maid_account_handler_.template Adjust<Data>(account_name, data_name, new_cost);
  }
};

TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest);

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_PutData) {
  typename TypeParam::name_type data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
  MaidName account_name(this->GenerateMaidName());
  int32_t cost(1000);
  std::unique_ptr<MaidAccount> account(
      new MaidAccount(account_name, *(this->vault_root_directory_)));

  EXPECT_THROW(this->PutData(account_name, data_name, cost), vault_error);
  this->AddAccount(std::move(account));
  // not enough space available
  EXPECT_THROW(this->PutData(account_name, data_name, cost), vault_error);

  this->SetTotalAvailable(account_name, cost * 2);
  EXPECT_NO_THROW(this->PutData(account_name, data_name, cost));
}

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_DeleteData) {
  typename TypeParam::name_type data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
  MaidName account_name(this->GenerateMaidName());
  std::unique_ptr<MaidAccount> account(
      new MaidAccount(account_name, *(this->vault_root_directory_)));

  EXPECT_THROW(this->DeleteData(account_name, data_name), vault_error);
  this->AddAccount(std::move(account));
  EXPECT_NO_THROW(this->DeleteData(account_name, data_name));
}

TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_Adjust) {
}

REGISTER_TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest,
                           BEH_PutData,
                           BEH_DeleteData,
                           BEH_Adjust);

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
                       OwnerDirectory,
                       GroupDirectory,
                       WorldDirectory> AllTypes;

INSTANTIATE_TYPED_TEST_CASE_P(All, MaidAccountHandlerTypedTest, AllTypes);


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
