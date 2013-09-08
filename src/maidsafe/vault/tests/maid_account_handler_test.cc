/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/maid_manager/handler.h"

#include <vector>

#include "maidsafe/common/test.h"

namespace maidsafe {

namespace vault {

namespace test {


class MaidAccountHandlerTest : public testing::Test {
 public:
  MaidAccountHandlerTest()
      : vault_root_directory_(maidsafe::test::CreateTestPath("MaidSafe_Test_MaidAccountHandler")),
        db_(),
        maid_account_handler_(db_, NodeId(NodeId::kRandomId)) {}

  ~MaidAccountHandlerTest() {}

  MaidName GenerateMaidName() {
    return MaidName(Identity(RandomAlphaNumericString(64)));
  }

  nfs::PmidRegistration GenerateRegistration(bool unregister) {
    passport::Anmaid anmaid;
    passport::Maid maid(anmaid);
    passport::Pmid pmid(maid);
    nfs::PmidRegistration registration(maid, pmid, unregister);
    return registration;
  }

  PmidRecord GeneratePmidRecord(const PmidName& name,
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

  PmidRecord GeneratePmidRecord(const PmidName& name) {
    PmidRecord new_record(name);
    new_record.stored_count = RandomUint32() % 100;
    new_record.stored_total_size = new_record.stored_count * (RandomUint32() % 10000);
    new_record.lost_count = RandomUint32() % 100;
    new_record.lost_total_size = new_record.lost_count * (RandomUint32() % 10000);
    new_record.claimed_available_size = RandomUint32() % 1000000;
    return new_record;
  }

//  void AddAccount(std::unique_ptr<MaidName> account) {
//    int total_accounts_pre_add = (maid_account_handler_.GetAccountNames()).size();

//    EXPECT_NO_THROW(maid_account_handler_.CreateAccount<>(std::move(account)));
//    std::vector<MaidName> account_names(maid_account_handler_.GetAccountNames());
//    EXPECT_EQ(total_accounts_pre_add + 1, account_names.size());
//  }

//  void SetTotalAvailable(const MaidName& account_name, int64_t total) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    if (itr != maid_account_handler_.maid_accounts_.end())
//      (*itr)->total_claimed_available_size_by_pmids_ = total;
//  }

//  void SetTotalPut(const MaidName& account_name, int64_t total) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    if (itr != maid_account_handler_.maid_accounts_.end())
//      (*itr)->total_put_data_ = total;
//  }

  //  sets up and registers account, returns pmid_totals for comparison purposes
//  std::vector<PmidTotals> SetupAndRegisterAccount(const MaidName& account_name,
//                                                  int num_pmid_totals) {
//    std::vector<PmidTotals> generated_pmid_totals;
//    int64_t total_available_size(0), total_put_data(0);

//    std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));
//    CreateAccount(std::move(account));

//    for (int i(0); i < num_pmid_totals; ++i) {
//      nfs::PmidRegistration registration(GenerateRegistration(false));
//      nfs::PmidRegistration::serialised_type serialised_registration(registration.Serialise());
//      PmidRecord pmid_record(GeneratePmidRecord(registration.pmid_name()));
//      PmidTotals pmid_totals(serialised_registration, pmid_record);
//      EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));
//      EXPECT_NO_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_record));
//      total_available_size += pmid_record.claimed_available_size;
//      total_put_data += pmid_record.stored_total_size;
//      generated_pmid_totals.push_back(pmid_totals);
//    }

//    SetTotalPut(account_name, total_put_data);
//    SetTotalAvailable(account_name, total_available_size);

//    return generated_pmid_totals;
//  }

  std::vector<boost::filesystem::path> GenerateArchiveEntries(const MaidName& /*account_name*/,
                                                              int /*number_of_entries*/) {
    std::vector<boost::filesystem::path> entries;
    // implement
    return entries;
  }

//  // Individual account attribute accessors
//  std::vector<PmidTotals> GetPmids(const MaidName& account_name) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    return (*itr)->pmid_totals_;
//  }

//  std::deque<MaidAccount::PutDataDetails> GetPutDataDetails(const MaidName& account_name) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    return (*itr)->recent_put_data_;
//  }

//  int64_t GetTotalPutData(const MaidName& account_name) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    return (*itr)->total_put_data_;
//  }

//  int64_t GetAvailableSize(const MaidName& account_name) {
//    auto itr(detail::FindAccount(maid_account_handler_.maid_accounts_, account_name));
//    return (*itr)->total_claimed_available_size_by_pmids_;
//  }

 protected:
  maidsafe::test::TestPath vault_root_directory_;
  Db db_;
  MaidAccountHandler maid_account_handler_;
};


//TEST_F(MaidAccountHandlerTest, BEH_AddAccount) {
//  std::vector<MaidName> account_names;
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));
//  std::unique_ptr<MaidAccount> duplicate(new MaidAccount(account_name, *vault_root_directory_));

//  AddAccount(std::move(new_account));
//  account_names = maid_account_handler_.GetAccountNames();
//  EXPECT_EQ(1, account_names.size());

//  EXPECT_FALSE(maid_account_handler_.AddAccount(std::move(duplicate)));
//  account_names = maid_account_handler_.GetAccountNames();
//  EXPECT_EQ(1, account_names.size());
//}

//TEST_F(MaidAccountHandlerTest, BEH_DeleteAccount) {
//  std::vector<MaidName> account_names;
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));

//  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));

//  AddAccount(std::move(new_account));

//  EXPECT_TRUE(maid_account_handler_.DeleteAccount(account_name));
//  account_names = maid_account_handler_.GetAccountNames();
//  EXPECT_EQ(0, account_names.size());
//}

//TEST_F(MaidAccountHandlerTest, BEH_RegisterPmid) {
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> new_account(new MaidAccount(account_name, *vault_root_directory_));
//  nfs::PmidRegistration registration(GenerateRegistration(false));
//  std::vector<PmidTotals> pmids;

//  EXPECT_THROW(maid_account_handler_.RegisterPmid(account_name, registration), vault_error);
//  AddAccount(std::move(new_account));
//  pmids = GetPmids(account_name);
//  EXPECT_EQ(0, pmids.size());
//  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));

//  pmids = GetPmids(account_name);
//  EXPECT_EQ(1, pmids.size());
//  EXPECT_NE(pmids.end(), std::find_if(pmids.begin(),
//                                      pmids.end(),
//                                      [&registration](const PmidTotals record) {
//                                        return registration.pmid_name() ==
//                                            record.pmid_record.pmid_name;
//                                      }));

//  // same pmid attempted to be registered more than once
//  int total_pre_duplicate_register = pmids.size();
//  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));
//  pmids = GetPmids(account_name);
//  EXPECT_EQ(total_pre_duplicate_register, pmids.size());
//}

//TEST_F(MaidAccountHandlerTest, BEH_UnregisterPmid) {
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));
//  PmidName pmid_name(Identity(RandomAlphaNumericString(64)));

//  EXPECT_THROW(maid_account_handler_.UnregisterPmid(account_name, pmid_name), vault_error);

//  AddAccount(std::move(account));
//  EXPECT_NO_THROW(maid_account_handler_.UnregisterPmid(account_name, pmid_name));

//  std::vector<PmidTotals> pmids = GetPmids(account_name);
//  EXPECT_EQ(pmids.end(), std::find_if(pmids.begin(),
//                                      pmids.end(),
//                                      [&pmid_name](const PmidTotals record) {
//                                        return pmid_name == record.pmid_record.pmid_name;
//                                      }));
//}

//TEST_F(MaidAccountHandlerTest, BEH_UpdatePmidTotals) {
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));

//  nfs::PmidRegistration registration(GenerateRegistration(false));
//  nfs::PmidRegistration::serialised_type serialised_registration(registration.Serialise());

//  PmidRecord pmid_record(GeneratePmidRecord(registration.pmid_name(), 2, 2000, 1, 1000, 15000));
//  PmidTotals pmid_totals(serialised_registration, pmid_record);

//  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), vault_error);
//  AddAccount(std::move(account));
//  EXPECT_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals), common_error);
//  EXPECT_NO_THROW(maid_account_handler_.RegisterPmid(account_name, registration));

//  std::vector<PmidTotals> pmids = GetPmids(account_name);
//  EXPECT_EQ(1, pmids.size());
//  for (auto& pmidtotals : pmids) {
//    EXPECT_EQ(pmid_record.pmid_name, pmidtotals.pmid_record.pmid_name);
//    EXPECT_EQ(0, pmidtotals.pmid_record.stored_count);
//    EXPECT_EQ(0, pmidtotals.pmid_record.stored_total_size);
//    EXPECT_EQ(0, pmidtotals.pmid_record.lost_count);
//    EXPECT_EQ(0, pmidtotals.pmid_record.lost_total_size);
//  }

//  EXPECT_NO_THROW(maid_account_handler_.UpdatePmidTotals(account_name, pmid_totals));

//  pmids = GetPmids(account_name);
//  EXPECT_EQ(1, pmids.size());
//  for (auto& pmidtotals : pmids) {
//    EXPECT_EQ(pmid_record.pmid_name, pmidtotals.pmid_record.pmid_name);
//    EXPECT_EQ(2, pmidtotals.pmid_record.stored_count);
//    EXPECT_EQ(2000, pmidtotals.pmid_record.stored_total_size);
//    EXPECT_EQ(1, pmidtotals.pmid_record.lost_count);
//    EXPECT_EQ(1000, pmidtotals.pmid_record.lost_total_size);
//  }
//}

//TEST_F(MaidAccountHandlerTest, BEH_GetAccountNames) {
//  std::vector<MaidName> generated_account_names, retrieved_account_names, removed_account_names;
//  int total_accounts(15);
//  for (int i(0); i < total_accounts; ++i) {
//    MaidName name(GenerateMaidName());
//    std::unique_ptr<MaidAccount> account(new MaidAccount(name, *vault_root_directory_));
//    generated_account_names.push_back(name);
//    AddAccount(std::move(account));
//  }
//  EXPECT_EQ(total_accounts, generated_account_names.size());

//  retrieved_account_names = maid_account_handler_.GetAccountNames();
//  EXPECT_EQ(total_accounts, retrieved_account_names.size());

//  for (int i(0); i < total_accounts; ++i)
//    EXPECT_EQ(generated_account_names[i], retrieved_account_names[i]);

//  for (int i(0); i < 5; ++i) {
//    EXPECT_TRUE(maid_account_handler_.DeleteAccount(generated_account_names[i]));
//    removed_account_names.push_back(generated_account_names[i]);
//  }

//  retrieved_account_names = maid_account_handler_.GetAccountNames();
//  EXPECT_EQ(10, retrieved_account_names.size());
//  for (auto& account : removed_account_names)
//    EXPECT_EQ(retrieved_account_names.end(), std::find_if(retrieved_account_names.begin(),
//                                                          retrieved_account_names.end(),
//                                                          [&account] (const MaidName& name) {
//                                                            return account == name;
//                                                          }));
//}

//TEST_F(MaidAccountHandlerTest, BEH_GetSerialisedAccount) {
//  MaidName account_name(GenerateMaidName());
//  int num_pmid_totals(10);
//  int64_t total_space_used(0), total_space_available(0);

//  EXPECT_THROW(maid_account_handler_.GetSerialisedAccount(account_name), vault_error);

//  std::vector<PmidTotals> generated_pmids(SetupAndRegisterAccount(account_name, num_pmid_totals));
//  for (auto& pmid : generated_pmids) {
//    total_space_used += pmid.pmid_record.stored_total_size;
//    total_space_available += pmid.pmid_record.claimed_available_size;
//  }

//  MaidAccount::serialised_type serialised_account(
//      maid_account_handler_.GetSerialisedAccount(account_name));
//  MaidAccount retrieved_account(serialised_account, *vault_root_directory_);
//  std::vector<PmidTotals> retrieved_pmids(GetPmids(retrieved_account.name()));

//  EXPECT_EQ(account_name, retrieved_account.name());
//  EXPECT_EQ(total_space_used, GetTotalPutData(retrieved_account.name()));
//  EXPECT_EQ(total_space_available, GetAvailableSize(retrieved_account.name()));
//  EXPECT_EQ(num_pmid_totals, retrieved_pmids.size());
//  for (auto& pmid : retrieved_pmids) {
//    auto generated_pmid = std::find_if(generated_pmids.begin(),
//                                       generated_pmids.end(),
//                                       [&pmid] (const PmidTotals& record) {
//                                         return pmid.pmid_record.pmid_name ==
//                                             record.pmid_record.pmid_name;
//                                      });
//    EXPECT_EQ(generated_pmid->serialised_pmid_registration, pmid.serialised_pmid_registration);
//    EXPECT_EQ(generated_pmid->pmid_record.stored_count, pmid.pmid_record.stored_count);
//    EXPECT_EQ(generated_pmid->pmid_record.stored_total_size, pmid.pmid_record.stored_total_size);
//    EXPECT_EQ(generated_pmid->pmid_record.lost_count, pmid.pmid_record.lost_count);
//    EXPECT_EQ(generated_pmid->pmid_record.lost_total_size, pmid.pmid_record.lost_total_size);
//    EXPECT_EQ(generated_pmid->pmid_record.claimed_available_size,
//              pmid.pmid_record.claimed_available_size);
//  }
//}

//TEST_F(MaidAccountHandlerTest, BEH_GetArchiveFileNames) {
//  MaidName account_name(GenerateMaidName());
//  int num_entries(20);
//  std::vector<boost::filesystem::path> retrieved_archive_entries,
//      generated_archive_entries(GenerateArchiveEntries(account_name, num_entries));

//  EXPECT_THROW(maid_account_handler_.GetArchiveFileNames(account_name), vault_error);

//  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));
//  AddAccount(std::move(account));

//  retrieved_archive_entries = maid_account_handler_.GetArchiveFileNames(account_name);
//  EXPECT_EQ(retrieved_archive_entries.size(), generated_archive_entries.size());
////  for (int i(0); i < num_entries; ++i) {
////    EXPECT_EQ(retrieved_archive_entries[i].string(), generated_archive_entries[i].string());
////  }

//  //  to finish.....
//}

//TEST_F(MaidAccountHandlerTest, BEH_GetArchiveFile) {
//}

//TEST_F(MaidAccountHandlerTest, BEH_PutArchiveFile) {
//  MaidName account_name(GenerateMaidName());
//  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, *vault_root_directory_));
//  boost::filesystem::path filename;
//  NonEmptyString content;

//  EXPECT_THROW(maid_account_handler_.PutArchiveFile(account_name, filename, content), vault_error);
//  AddAccount(std::move(account));
//  EXPECT_NO_THROW(maid_account_handler_.PutArchiveFile(account_name, filename, content));

//  //  to finish.....
//}

//template <typename Data>
//class MaidAccountHandlerTypedTest : public MaidAccountHandlerTest {
// public:
//  MaidAccountHandlerTypedTest()
//    : MaidAccountHandlerTest() {}

// protected:
//  void PutData(const MaidName& account_name,
//               const typename Data::Name& data_name,
//               int32_t cost) {
//    this->maid_account_handler_.template PutData<Data>(account_name,
//                                                       data_name,
//                                                       cost);
//  }

//  void DeleteData(const MaidName& account_name, const typename Data::Name& data_name) {
//    this->maid_account_handler_.template DeleteData<Data>(account_name, data_name);
//  }

//  void Adjust(const MaidName& account_name,
//              const typename Data::Name& data_name,
//              int32_t new_cost)
//  {
//    this->maid_account_handler_.template Adjust<Data>(account_name, data_name, new_cost);
//  }

//  void CheckPutDetails(const MaidName& account_name,
//                       typename Data::Name data_name,
//                       int32_t cost,
//                       int size) {
//    std::deque<MaidAccount::PutDataDetails> put_details(this->GetPutDataDetails(account_name));
//    EXPECT_EQ(size, put_details.size());

//    auto itr(std::find_if(put_details.begin(), put_details.end(),
//                          [&data_name] (const MaidAccount::PutDataDetails& record) {
//                            return DataNameVariant(data_name) == record.data_name_variant;
//                          }));
//    if (itr != put_details.end())
//      EXPECT_EQ(itr->cost, cost);
//  }
//};

//TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest);

//TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_PutData) {
//  typename TypeParam::Name data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
//  MaidName account_name(this->GenerateMaidName());
//  int32_t cost(1000);
//  std::unique_ptr<MaidAccount> account(
//      new MaidAccount(account_name, *(this->vault_root_directory_)));

//  EXPECT_THROW(this->PutData(account_name, data_name, cost), vault_error);
//  this->AddAccount(std::move(account));
//  // not enough space available
//  EXPECT_THROW(this->PutData(account_name, data_name, cost), vault_error);

//  this->SetTotalAvailable(account_name, cost * 2);

//  EXPECT_NO_THROW(this->PutData(account_name, data_name, cost));
//  EXPECT_EQ(cost, this->GetTotalPutData(account_name));
//  this->CheckPutDetails(account_name, data_name, cost, 1);
//}

//TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_DeleteData) {
//  typename TypeParam::Name data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
//  int32_t cost(1000);
//  MaidName account_name(this->GenerateMaidName());
//  std::unique_ptr<MaidAccount> account(
//      new MaidAccount(account_name, *(this->vault_root_directory_)));

//  EXPECT_THROW(this->DeleteData(account_name, data_name), vault_error);
//  this->AddAccount(std::move(account));
//  EXPECT_NO_THROW(this->DeleteData(account_name, data_name));

//  this->SetTotalAvailable(account_name, cost * 2);
//  EXPECT_NO_THROW(this->PutData(account_name, data_name, cost));
//  EXPECT_EQ(cost, this->GetTotalPutData(account_name));
//  this->CheckPutDetails(account_name, data_name, cost, 1);

//  EXPECT_NO_THROW(this->DeleteData(account_name, data_name));
//  this->CheckPutDetails(account_name, data_name, 0, 0);
//  EXPECT_EQ(0, this->GetTotalPutData(account_name));
//}

//TYPED_TEST_P(MaidAccountHandlerTypedTest, BEH_Adjust) {
//  typename TypeParam::Name data_name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
//  int32_t old_cost(1000), new_cost_small(500), new_cost_large(20000);
//  MaidName account_name(this->GenerateMaidName());
//  std::unique_ptr<MaidAccount> account(
//        new MaidAccount(account_name, *(this->vault_root_directory_)));

//  EXPECT_THROW(this->Adjust(account_name, data_name, new_cost_small), vault_error);

//  this->AddAccount(std::move(account));
//  this->SetTotalAvailable(account_name, old_cost);
//  EXPECT_NO_THROW(this->PutData(account_name, data_name, old_cost));
//  EXPECT_EQ(old_cost, this->GetTotalPutData(account_name));
//  this->CheckPutDetails(account_name, data_name, old_cost, 1);

//  EXPECT_NO_THROW(this->Adjust(account_name, data_name, new_cost_small));
//  EXPECT_EQ(new_cost_small, this->GetTotalPutData(account_name));
//  this->CheckPutDetails(account_name, data_name, new_cost_small, 1);

//  // not enough space, so Adjust should fail, nothing should change.
//  EXPECT_THROW(this->Adjust(account_name, data_name, new_cost_large), vault_error);
//  EXPECT_EQ(new_cost_small, this->GetTotalPutData(account_name));
//  this->CheckPutDetails(account_name, data_name, new_cost_small, 1);

//  // enough space this time - Adjust should succeed and changes committed.
//  this->SetTotalAvailable(account_name, new_cost_large);
//  EXPECT_NO_THROW(this->Adjust(account_name, data_name, new_cost_large));
//  this->CheckPutDetails(account_name, data_name, new_cost_large, 1);
//}

//REGISTER_TYPED_TEST_CASE_P(MaidAccountHandlerTypedTest, BEH_PutData, BEH_DeleteData, BEH_Adjust);

//typedef testing::Types<passport::PublicAnmid,
//                       passport::PublicAnsmid,
//                       passport::PublicAntmid,
//                       passport::PublicAnmaid,
//                       passport::PublicMaid,
//                       passport::PublicPmid,
//                       passport::Mid,
//                       passport::Smid,
//                       passport::Tmid,
//                       passport::PublicAnmpid,
//                       passport::PublicMpid,
//                       ImmutableData,
//                       OwnerDirectory,
//                       GroupDirectory,
//                       WorldDirectory> AllTypes;

//INSTANTIATE_TYPED_TEST_CASE_P(All, MaidAccountHandlerTypedTest, AllTypes);


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
