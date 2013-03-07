/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/metadata_manager/metadata_handler.h"

#include <vector>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/metadata_manager/metadata_pb.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace test {

int kOnlinePmids(10);
int kOfflinePmids(20);

Identity GenerateIdentity() {
  return Identity(RandomAlphaNumericString(crypto::SHA512::DIGESTSIZE));
}

class MetadataHandlerTest : public testing::Test {
 public:
  MetadataHandlerTest()
    : vault_root_dir_(maidsafe::test::CreateTestPath(("MaidSafe_Test_MetadataHandler"))),
      vault_metadata_dir_(*vault_root_dir_ / "metadata"),
      metadata_handler_(*vault_root_dir_),
      online_pmids_(),
      offline_pmids_() {}

  void SetUp() {
    online_pmids_ = GeneratePmids(kOnlinePmids);
    offline_pmids_ = GeneratePmids(kOfflinePmids);
  }

 protected:
  std::vector<Identity> GeneratePmids(int total_pmids) {
    std::vector<Identity> pmids;
    for (int i(0); i < total_pmids; ++i) {
      pmids.push_back(GenerateIdentity());
    }
    return pmids;
  }

  const maidsafe::test::TestPath vault_root_dir_;
  fs::path vault_metadata_dir_;
  MetadataHandler metadata_handler_;
  std::vector<Identity> online_pmids_, offline_pmids_;
};

TEST_F(MetadataHandlerTest, BEH_Constructor) {
  fs::path vault_root(*vault_root_dir_ / "constructor_test");
  {
    try {
      MetadataHandler metadata_handler(vault_root);
      FAIL() << "Object construction should throw with non-existent path";
    }
    catch(const std::exception&) {}
    fs::create_directory(vault_root);
    fs::path metadata_path(vault_root / "metadata");
    ASSERT_TRUE(WriteFile(metadata_path, RandomString(64)));
    try {
      MetadataHandler metadata_handler(vault_root);
      FAIL() << "Object construction should throw with metadata dir being present as file";
    }
    catch(const common_error& error) {
      EXPECT_EQ(error.code(), CommonErrors::not_a_directory);
    }
  }
  fs::path valid_root(vault_root / "valid");
  fs::create_directory(valid_root);
  EXPECT_NO_THROW(MetadataHandler metadata_handler(valid_root));
  EXPECT_TRUE(fs::exists(valid_root / "metadata"));
  EXPECT_TRUE(fs::is_directory(valid_root / "metadata"));
}

template <typename Data>
class MetadataHandlerTypedTest : public MetadataHandlerTest {
 public :
  MetadataHandlerTypedTest() : MetadataHandlerTest() {}

 protected :
  void IncrementSubscribers(const typename Data::name_type& data_name, int32_t data_size) {
    this->metadata_handler_.template IncrementSubscribers<Data>(data_name, data_size);
  }

  void DecrementSubscribers(const typename Data::name_type& data_name) {
    this->metadata_handler_.template DecrementSubscribers<Data>(data_name);
  }

  void DeleteMetadata(const typename Data::name_type& data_name) {
    this->metadata_handler_.template DeleteMetadata<Data>(data_name);
  }

  void MarkNodeDown(const typename Data::name_type& data_name,
                    const PmidName& pmid_name,
                    int& remaining_online_holders) {
    this->metadata_handler_.template MarkNodeDown<Data>(data_name,
                                                        pmid_name,
                                                        remaining_online_holders);
  }

  void MarkNodeUp(const typename Data::name_type& data_name,
                  const PmidName& pmid_name) {
    this->metadata_handler_.template MarkNodeUp<Data>(data_name, pmid_name);
  }

  void AddDataHolder(const typename Data::name_type& data_name,
                     const PmidName& online_pmid_name) {
    this->metadata_handler_.template AddDataHolder<Data>(data_name, online_pmid_name);
  }

  void RemoveDataHolder(const typename Data::name_type& data_name,
                        const PmidName& pmid_name) {
    this->metadata_handler_.template RemoveDataHolder<Data>(data_name, pmid_name);
  }

  std::vector<PmidName> GetOnlineDataHolders(const typename Data::name_type& data_name) const {
    return this->metadata_handler_.template GetOnlineDataHolders<Data>(data_name);
  }

  void CheckMetadataExists(const typename Data::name_type& data_name) const {
    this->metadata_handler_.template CheckMetadataExists<Data>(data_name);
  }

  void CompareMetadata(const typename Data::name_type& data_name,
                       int32_t data_size,
                       int64_t subscriber_count) {
    MetadataHandler::Metadata<Data> metadata(data_name, vault_metadata_dir_);
    EXPECT_EQ(metadata.content.name(), data_name->string());
    EXPECT_EQ(metadata.content.size(), data_size);
    EXPECT_EQ(metadata.content.subscribers(), subscriber_count);
  }

  void CompareMetadata(const typename Data::name_type& data_name,
                       int32_t data_size,
                       int64_t subscriber_count,
                       const std::vector<Identity>& online_pmids,
                       const std::vector<Identity>& offline_pmids) {
    MetadataHandler::Metadata<Data> metadata(data_name, vault_metadata_dir_);
    EXPECT_EQ(metadata.content.type(), static_cast<int>(Data::type_enum_value()));
    EXPECT_EQ(metadata.content.name(), data_name->string());
    EXPECT_EQ(metadata.content.size(), data_size);
    EXPECT_EQ(metadata.content.subscribers(), subscriber_count);
    for (uint32_t i(0); i < online_pmids.size(); ++i)
      EXPECT_EQ(metadata.content.online_pmid_name(i), online_pmids[i].string());
    for (uint32_t i(0); i < offline_pmids.size(); ++i)
      EXPECT_EQ(metadata.content.offline_pmid_name(i), offline_pmids[i].string());
  }

  void InsertMetadata(const typename Data::name_type& data_name,
                      int32_t data_size,
                      int subscriber_count,
                      const std::vector<Identity>& online_pmids,
                      const std::vector<Identity>& offline_pmids) {
    MetadataHandler::Metadata<Data> metadata(data_name, vault_metadata_dir_, data_size);
    metadata.content.set_subscribers(subscriber_count);
    for (auto& pmid : online_pmids)
      metadata.content.add_online_pmid_name(pmid.string());
    for (auto& pmid : offline_pmids)
      metadata.content.add_offline_pmid_name(pmid.string());
    metadata.SaveChanges();
  }

  protobuf::Metadata CreateProtoMetadata(const typename Data::name_type& data_name,
                                         const std::vector<Identity>& online_pmids,
                                         const std::vector<Identity>& offline_pmids) {
    protobuf::Metadata metadata;
    metadata.set_type(static_cast<int>(Data::type_enum_value()));
    metadata.set_name(data_name->string());
    metadata.set_size(RandomUint32());
    metadata.set_subscribers(RandomUint32() % 20);
    for (auto& pmid : online_pmids)
      metadata.add_online_pmid_name(pmid.string());
    for (auto& pmid : offline_pmids)
      metadata.add_offline_pmid_name(pmid.string());
    return metadata;
  }

//  bool CheckDataExistenceAndParsing(const Identity& data_name,
//                                    protobuf::Metadata& element) {
//    if (!boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name))) {
//      LOG(kError) << "Data was not found.";
//      return false;
//    }
//    NonEmptyString serialised_element(ReadFile(vault_metadata_dir_ / EncodeToBase64(data_name)));
//    if (!element.ParseFromString(serialised_element.string())) {
//      LOG(kError) << "Data did not parse.";
//      return false;
//    }
//    return true;
//  }

//  bool CheckPmidIds(protobuf::Metadata& element,
//                    const std::set<PmidName>& online_pmids,
//                    const std::set<PmidName>& offline_pmids) {
//    std::set<PmidName> element_online_pmid_names;
//    for (uint16_t i(0); i < element.online_pmid_name_size(); ++i) {
//      try {
//        PmidName online_pmid_name(Identity(element.online_pmid_name(i)));
//        element_online_pmid_names.insert(online_pmid_name);
//      } catch(const std::exception& ex) {
//        LOG(kError) << "Data has online pmid of wrong size.  " << ex.what();
//        return false;
//      }
//    }

//    if (online_pmids != element_online_pmid_names) {
//      LOG(kError) << "Data's online pmid IDs don't match expectation.";
//      LOG(kVerbose) << "\n\nonline - expected:";
//      for (auto pmid : online_pmids)
//        LOG(kVerbose) << pmid->string();
//      LOG(kVerbose) << "\nonline - actual:";
//      for (auto pmid : element_online_pmid_names)
//        LOG(kVerbose) << pmid->string();
//      return false;
//    }

//    std::set<PmidName> element_offline_pmid_names;
//    for (uint16_t i(0); i < element.offline_pmid_name_size(); ++i) {
//      try {
//        PmidName offline_pmid_name(Identity(element.offline_pmid_name(i)));
//        element_offline_pmid_names.insert(offline_pmid_name);
//      } catch(const std::exception& ex) {
//        LOG(kError) << "Data has offline pmid of wrong size.  " << ex.what();
//        return false;
//      }
//    }
//    if (offline_pmids != element_offline_pmid_names) {
//      LOG(kError) << "Data's offline pmid IDs don't match expectation.";
//      LOG(kVerbose) << "\n\noffline - expected:";
//      for (auto pmid : offline_pmids)
//        LOG(kVerbose) << pmid->string();
//      LOG(kVerbose) << "\noffline - actual:";
//      for (auto pmid : element_offline_pmid_names)
//        LOG(kVerbose) << pmid->string();
//      return false;
//    }

//    return true;
//  }

//  bool CheckPmidIds(protobuf::Metadata& element,
//                    const PmidName& online_pmid,
//                    const PmidName& offline_pmid) {
//    if (element.online_pmid_name_size() != 1) {
//      LOG(kError) << "Expected one online pmid, found " << element.online_pmid_name_size();
//      return false;
//    }

//    if (element.offline_pmid_name_size() != 1) {
//      LOG(kError) << "Expected one offline pmid, found " << element.offline_pmid_name_size();
//      return false;
//    }

//    if (element.online_pmid_name(0) != online_pmid->string()) {
//      LOG(kError) << "Data's online pmid is wrong.";
//      return false;
//    }

//    if (element.offline_pmid_name(0) != offline_pmid->string()) {
//      LOG(kError) << "Data's offline pmid is wrong.";
//      return false;
//    }

//    return true;
//  }

//  bool CheckDataExistenceAndIntegrity(const Identity& data_name,
//                                      const int32_t& size,
//                                      const int64_t& subscribers,
//                                      const std::set<PmidName>& online_pmids,
//                                      const std::set<PmidName>& offline_pmids) {
//    protobuf::Metadata element;
//    if (!CheckDataExistenceAndParsing(data_name, element))
//      return false;

//    if (element.size() != size) {
//      LOG(kError) << "Data has wrong size.";
//      return false;
//    }

//    if (element.subscribers() != subscribers) {
//      LOG(kError) << "Wrong number stored.";
//      return false;
//    }

//    if (online_pmids.empty() && (element.online_pmid_name_size() != 0)) {
//      LOG(kError) << "Data has too many online pmid IDs.";
//      return false;
//    }
//    if (offline_pmids.empty() && (element.offline_pmid_name_size() != 0)) {
//      LOG(kError) << "Data has too many offline pmid IDs.";
//      return false;
//    }

//    if (!CheckPmidIds(element, online_pmids, offline_pmids))
//      return false;

//    return true;
//  }

//  bool CheckDataExistenceAndIntegrity(const Identity& data_name,
//                                      const int32_t &size,
//                                      const int64_t& subscribers,
//                                      const PmidName& online_pmid,
//                                      const PmidName& offline_pmid) {
//    protobuf::Metadata element;
//    if (!CheckDataExistenceAndParsing(data_name, element))
//      return false;

//    if (element.size() != size) {
//      LOG(kError) << "Data has wrong size.";
//      return false;
//    }

//    if (element.subscribers() != subscribers) {
//      LOG(kError) << "Wrong number stored.";
//      return false;
//    }

//    if (!CheckPmidIds(element, online_pmid, offline_pmid))
//      return false;

//    return true;
//  }

//  void AddPmidIds(const typename Data::name_type& data_name,
//                  const int32_t& size,
//                  const uint16_t& max_value,
//                  const int64_t subscribers,
//                  std::set<PmidName>& online_pmid_names,
//                  std::set<PmidName>& offline_pmid_names,
//                  std::vector<PmidName>& online_for_change,
//                  std::vector<PmidName>& offline_for_change,
//                  int64_t& expected_holders) {
//    for (uint16_t i(0); i < max_value; ++i) {
//      PmidName online_pmid_name(GenerateIdentity());
//      this->metadata_handler_.AddDataHolder(data_name, online_pmid_name);
//      online_pmid_names.insert(online_pmid_name);
//      ++expected_holders;
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name,
//                                                 size,
//                                                 subscribers,
//                                                 online_pmid_names,
//                                                 offline_pmid_names));
//      if (RandomUint32() % 3 == 0)
//        online_for_change.push_back(online_pmid_name);

//      PmidName offline_pmid_name(GenerateIdentity());
//      this->metadata_handler_.AddOfflinePmid(data_name, offline_pmid_name);
//      offline_pmid_names.insert(offline_pmid_name);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name,
//                                                 size,
//                                                 subscribers,
//                                                 online_pmid_names,
//                                                 offline_pmid_names));
//      if (RandomUint32() % 3 == 0)
//        offline_for_change.push_back(offline_pmid_name);
//    }
//  }

//  void AddPmidIds(const typename Data::name_type& data_name,
//                  const int32_t& size,
//                  const uint16_t& max_value,
//                  const int64_t subscribers,
//                  std::set<PmidName>& online_pmid_names,
//                  std::set<PmidName>& offline_pmid_names) {
//    for (uint16_t i(0); i < max_value; ++i) {
//      PmidName online_pmid_name(GenerateIdentity());
//      this->metadata_handler_.AddDataHolder(data_name, online_pmid_name);
//      online_pmid_names.insert(online_pmid_name);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name->String(),
//                                                 size,
//                                                 subscribers,
//                                                 online_pmid_names,
//                                                 offline_pmid_names));

//      PmidName offline_pmid_name(GenerateIdentity());
//      this->metadata_handler_.AddOfflinePmid(data_name, offline_pmid_name);
//      offline_pmid_names.insert(offline_pmid_name);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name->String(),
//                                                 size,
//                                                 subscribers,
//                                                 online_pmid_names,
//                                                 offline_pmid_names));
//    }
//  }
};

TYPED_TEST_CASE_P(MetadataHandlerTypedTest);

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_IncrementSubscribers) {
  typename TypeParam::name_type data_name(GenerateIdentity());
  int32_t data_size(10000);
  this->IncrementSubscribers(data_name, data_size);
  this->CompareMetadata(data_name, data_size, 1);

  EXPECT_THROW(this->IncrementSubscribers(data_name, data_size + 1), common_error);
  this->CompareMetadata(data_name, data_size, 1);
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_DecrementSubscribers) {
  typename TypeParam::name_type data_name(GenerateIdentity());
  int32_t data_size(10000);

  EXPECT_THROW(this->DecrementSubscribers(data_name), common_error);
  this->InsertMetadata(data_name, data_size, 2, this->online_pmids_, this->offline_pmids_);
  this->CompareMetadata(data_name, data_size, 2, this->online_pmids_, this->offline_pmids_);

  this->DecrementSubscribers(data_name);
  this->CompareMetadata(data_name, data_size, 1, this->online_pmids_, this->offline_pmids_);

  this->DecrementSubscribers(data_name);
  EXPECT_THROW(this->DecrementSubscribers(data_name), common_error);
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_PutMetadata) {
  protobuf::Metadata metadata, replacement_metadata;
  typename TypeParam::name_type data_name(GenerateIdentity());
  EXPECT_THROW(this->metadata_handler_.PutMetadata(metadata), common_error);

  metadata = this->CreateProtoMetadata(data_name, this->online_pmids_, this->offline_pmids_);

  EXPECT_NO_THROW(this->metadata_handler_.PutMetadata(metadata));
  this->CompareMetadata(data_name,
                        metadata.size(),
                        metadata.subscribers(),
                        this->online_pmids_,
                        this->offline_pmids_);

  // create new proto::metadata and call Put - previous entry should be overwritten
  replacement_metadata = this->CreateProtoMetadata(data_name,
                                                   this->offline_pmids_,
                                                   this->online_pmids_);
  EXPECT_NO_THROW(this->metadata_handler_.PutMetadata(replacement_metadata));
  this->CompareMetadata(data_name,
                        replacement_metadata.size(),
                        replacement_metadata.subscribers(),
                        this->offline_pmids_,
                        this->online_pmids_);
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_DeleteMetadata) {
  typename TypeParam::name_type data_name(GenerateIdentity());
  int32_t data_size(10000);

  EXPECT_THROW(this->DeleteMetadata(data_name), common_error);
  this->InsertMetadata(data_name, data_size, 10, this->online_pmids_, this->offline_pmids_);
  this->CompareMetadata(data_name, data_size, 10, this->online_pmids_, this->offline_pmids_);

  EXPECT_NO_THROW(this->DeleteMetadata(data_name));
  // compare should throw as metadata removed when subscribers = 0, so should fail to parse
  EXPECT_THROW(this->CompareMetadata(data_name,
                                     data_size,
                                     0,
                                     this->online_pmids_,
                                     this->offline_pmids_),
               common_error);

  // attempt to add new metadata under same name - should succeed
  EXPECT_NO_THROW(this->InsertMetadata(data_name,
                                       data_size + 1,
                                       3,
                                       this->offline_pmids_,
                                       this->online_pmids_));
  this->CompareMetadata(data_name, data_size+1, 3, this->offline_pmids_, this->online_pmids_);
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_MarkNodeDown) {
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_MarkNodeUp) {
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_AddDataHolder) {
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_RemoveDataHolder) {
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_GetOnlineDataHolders) {
}

TYPED_TEST_P(MetadataHandlerTypedTest, BEH_CheckMetadataExists) {
}


REGISTER_TYPED_TEST_CASE_P(MetadataHandlerTypedTest,
                           BEH_IncrementSubscribers,
                           BEH_DecrementSubscribers,
                           BEH_PutMetadata,
                           BEH_DeleteMetadata,
                           BEH_MarkNodeDown,
                           BEH_MarkNodeUp,
                           BEH_AddDataHolder,
                           BEH_RemoveDataHolder,
                           BEH_GetOnlineDataHolders,
                           BEH_CheckMetadataExists);

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

INSTANTIATE_TYPED_TEST_CASE_P(All, MetadataHandlerTypedTest, AllTypes);

//class MetadataHandlerOneElementTest : public MetadataHandlerTest {
// public:
//  MetadataHandlerOneElementTest()
//      : data_name_(GenerateIdentity()),
//        size_(RandomInt32()),
//        subscribers_(0),
//        online_pmid_name_(GenerateIdentity()),
//        offline_pmid_name_(GenerateIdentity()) {}

// protected:
//  void SetUp() {
//    online_pmid_names_.insert(online_pmid_name_);
//    offline_pmid_names_.insert(offline_pmid_name_);
//    metadata_handler_.AddDataElement(data_name_, size_, online_pmid_name_, offline_pmid_name_);
//    ++subscribers_;

//    EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                               size_,
//                                               subscribers_,
//                                               online_pmid_names_,
//                                               offline_pmid_names_));
//    LOG(kInfo) << "SetUp complete!";
//  }

// public:
//  Identity data_name_;
//  int32_t size_;
//  int64_t subscribers_;
//  PmidName online_pmid_name_;
//  PmidName offline_pmid_name_;
//  std::set<PmidName> online_pmid_names_;
//  std::set<PmidName> offline_pmid_names_;
//};

//TEST_F(MetadataHandlerOneElementTest, BEH_ReAddDataElement) {
//  // Re-add the added element and check it again
//  for (uint16_t i(0); i < 20; ++i) {
//    metadata_handler_.AddDataElement(data_name_, size_, online_pmid_name_, offline_pmid_name_);
//    ++subscribers_;

//    EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                               size_,
//                                               subscribers_,
//                                               online_pmid_names_,
//                                               offline_pmid_names_));
//  }
//}

//TEST_F(MetadataHandlerOneElementTest, DISABLED_BEH_AddChangedDataElement) {
//  // TODO(Alison) - this test currently check that, when two elements with the same data name
//  // are added, it is the first element that stays (the first element is added in the test setup).
//  // This test should be updated when it has been decided if this is the correct behaviour.
//  for (uint16_t i(0); i < 20; ++i) {
//    metadata_handler_.AddDataElement(data_name_,
//                                          RandomInt32(),
//                                          PmidName(GenerateIdentity()),
//                                          PmidName(GenerateIdentity()));
//    ++subscribers_;

//    EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                               size_,
//                                               subscribers_,
//                                               online_pmid_names_,
//                                               offline_pmid_names_));
//  }
//}

//TEST_F(MetadataHandlerOneElementTest, BEH_AddAndRemoveDataElement) {
//  for (uint16_t i(0); i < 5; ++i) {
//    EXPECT_NO_THROW(metadata_handler_.RemoveDataElement(data_name_));
//    subscribers_ = 0;
//    EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name_)));

//    for (uint16_t j(0); j < 1 + RandomUint32() % 20; ++j) {
//      metadata_handler_.AddDataElement(data_name_,
//                                            size_,
//                                            online_pmid_name_,
//                                            offline_pmid_name_);
//      ++subscribers_;
//      EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                                 size_,
//                                                 subscribers_,
//                                                 online_pmid_names_,
//                                                 offline_pmid_names_));
//    }
//  }
//}

//TEST_F(MetadataHandlerTest, BEH_NonexistentDataElement) {
//  Identity data_name(GenerateIdentity());
//  PmidName pmid_name(GenerateIdentity());
//  int64_t holders;

//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  // TODO(Alison) - sensitise to CommonErrors::no_such_element?
//  EXPECT_THROW(metadata_handler_.RemoveDataElement(data_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.AddDataHolder(data_name, pmid_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.AddOfflinePmid(data_name, pmid_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.RemoveOnlinePmid(data_name, pmid_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.RemoveOfflinePmid(data_name, pmid_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.MoveNodeToOffline(data_name, pmid_name, holders),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

//  EXPECT_THROW(metadata_handler_.MoveNodeToOnline(data_name, pmid_name),
//               std::exception);
//  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));
//}

//TEST_F(MetadataHandlerOneElementTest, BEH_AddRemovePmids) {
//  std::vector<PmidName> online_for_removal;
//  std::vector<PmidName> offline_for_removal;
//  int64_t expected_holders(1);
//  uint16_t num_pmids_to_add(20);  // TODO(Alison) - preferred value of this?
//  AddPmidIds(data_name_,
//             size_,
//             num_pmids_to_add,
//             subscribers_,
//             online_pmid_names_,
//             offline_pmid_names_,
//             online_for_removal,
//             offline_for_removal,
//             expected_holders);

//  // Remove some (try to randomise orders a bit)
//  while (!online_for_removal.empty() && !offline_for_removal.empty()) {
//    if (!online_for_removal.empty() && (RandomUint32() % 4 != 0)) {
//      size_t index(RandomUint32() % online_for_removal.size());
//      PmidName to_remove(online_for_removal.at(index));
//      metadata_handler_.RemoveOnlinePmid(data_name_, to_remove);
//      online_pmid_names_.erase(to_remove);
//      online_for_removal.erase(online_for_removal.begin() + index);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                                 size_,
//                                                 subscribers_,
//                                                 online_pmid_names_,
//                                                 offline_pmid_names_));
//    }

//    if (!offline_for_removal.empty() && (RandomUint32() % 4 != 0)) {
//      size_t index(RandomUint32() % offline_for_removal.size());
//      PmidName to_remove(offline_for_removal.at(index));
//      metadata_handler_.RemoveOfflinePmid(data_name_, to_remove);
//      offline_pmid_names_.erase(to_remove);
//      offline_for_removal.erase(offline_for_removal.begin() + index);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                                 size_,
//                                                 subscribers_,
//                                                 online_pmid_names_,
//                                                 offline_pmid_names_));
//    }
//  }

//  // Add some more
//  AddPmidIds(data_name_,
//             size_,
//             num_pmids_to_add,
//             subscribers_,
//             online_pmid_names_,
//             offline_pmid_names_);
//}

//TEST_F(MetadataHandlerOneElementTest, BEH_MovePmids) {
//  // Populate with online/offline pmid ids, and randomly select some for moving later
//  std::vector<PmidName> online_for_moving;
//  std::vector<PmidName> offline_for_moving;
//  int64_t expected_holders(1);  // include initially added online_pmid_name_
//  uint16_t num_pmids_to_add(20);  // TODO(Alison) - preferred value of this?
//  AddPmidIds(data_name_,
//             size_,
//             num_pmids_to_add,
//             subscribers_,
//             online_pmid_names_,
//             offline_pmid_names_,
//             online_for_moving,
//             offline_for_moving,
//             expected_holders);

//  // Move pmid ids between online and offline (try to randomise orders a bit)
//  int64_t holders(0);
//  while (!online_for_moving.empty() && !offline_for_moving.empty()) {
//    if (!online_for_moving.empty() && (RandomUint32() % 4 != 0)) {
//      size_t index(RandomUint32() % online_for_moving.size());
//      PmidName to_move(online_for_moving.at(index));
//      metadata_handler_.MoveNodeToOffline(data_name_, to_move, holders);
//      --expected_holders;
//      EXPECT_EQ(expected_holders, holders);
//      online_pmid_names_.erase(to_move);
//      offline_pmid_names_.insert(to_move);
//      online_for_moving.erase(online_for_moving.begin() + index);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                                 size_,
//                                                 subscribers_,
//                                                 online_pmid_names_,
//                                                 offline_pmid_names_));
//    }

//    if (!offline_for_moving.empty() && (RandomUint32() % 4 != 0)) {
//      size_t index(RandomUint32() % offline_for_moving.size());
//      PmidName to_move(offline_for_moving.at(index));
//      metadata_handler_.MoveNodeToOnline(data_name_, to_move);
//      ++expected_holders;
//      offline_pmid_names_.erase(to_move);
//      online_pmid_names_.insert(to_move);
//      offline_for_moving.erase(offline_for_moving.begin() + index);
//      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
//                                                 size_,
//                                                 subscribers_,
//                                                 online_pmid_names_,
//                                                 offline_pmid_names_));
//    }
//  }

//  // Add some more
//  AddPmidIds(data_name_,
//             size_,
//             num_pmids_to_add,
//             subscribers_,
//             online_pmid_names_,
//             offline_pmid_names_);
//}

//TEST_F(MetadataHandlerTest, BEH_ManyDataElements) {
//  // Add and remove many data elements in various combinations
//  std::vector<Identity> data_names;
//  std::vector<int32_t> sizes;
//  std::vector<PmidName> online_pmid_names;
//  std::vector<PmidName> offline_pmid_names;

//  // Add elements
//  uint16_t max(1000);
//  for (uint16_t i(0); i < max; ++i) {
//    data_names.push_back(GenerateIdentity());
//    sizes.push_back(RandomInt32());
//    online_pmid_names.push_back(PmidName(GenerateIdentity()));
//    offline_pmid_names.push_back(PmidName(GenerateIdentity()));
//    metadata_handler_.AddDataElement(data_names.back(),
//                                          sizes.back(),
//                                          online_pmid_names.back(),
//                                          offline_pmid_names.back());
//  }

//  // Check existence
//  for (uint16_t i(0); i < max; ++i) {
//    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
//                                               sizes.at(i),
//                                               1,
//                                               online_pmid_names.at(i),
//                                               offline_pmid_names.at(i)));
//  }

//  // Remove some elements
//  for (uint16_t i(0); i < max; i += 2) {
//    metadata_handler_.RemoveDataElement(data_names.at(i));
//  }

//  // Check non-existence and existence
//  for (uint16_t i(0); i < max; i += 2)
//    EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_names.at(i))));

//  for (uint16_t i(1); i < max; i += 2) {
//    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
//                                               sizes.at(i),
//                                               1,
//                                               online_pmid_names.at(i),
//                                               offline_pmid_names.at(i)));
//  }

//  // Re-add removed elements
//  for (uint16_t i(0); i < max; i += 2) {
//    metadata_handler_.AddDataElement(data_names.at(i),
//                                          sizes.at(i),
//                                          online_pmid_names.at(i),
//                                          offline_pmid_names.at(i));
//  }

//  // Check existence
//  for (uint16_t i(0); i < max; ++i) {
//    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
//                                               sizes.at(i),
//                                               1,
//                                               online_pmid_names.at(i),
//                                               offline_pmid_names.at(i)));
//  }
//}

//TEST_F(MetadataHandlerTest, DISABLED_BEH_RandomProtobuf) {
//  // TODO(Alison) - This test currently places a random file in memory, then checks that the
//  // metadata_handler_ throws when it tries to use the file. It then checks that calling
//  // AddDataElement for an element with the same data_name overwrites the 'corrupt' file, and
//  // nothing should thrown any more.
//  // This test should be updated when it has been decided if this is the correct functionality.
//  Identity data_name(GenerateIdentity());

//  WriteFile(vault_metadata_dir_ / EncodeToBase64(data_name.string()),
//            RandomAlphaNumericString(RandomUint32() % 100));
//  EXPECT_THROW(metadata_handler_.AddDataHolder(data_name, PmidName(GenerateIdentity())),
//               std::exception);
//  EXPECT_THROW(metadata_handler_.AddOfflinePmid(data_name, PmidName(GenerateIdentity())),
//               std::exception);
//  EXPECT_THROW(metadata_handler_.RemoveOnlinePmid(data_name, PmidName(GenerateIdentity())),
//               std::exception);
//  EXPECT_THROW(metadata_handler_.RemoveOfflinePmid(data_name, PmidName(GenerateIdentity())),
//               std::exception);
//  int64_t holders;
//  EXPECT_THROW(metadata_handler_.MoveNodeToOffline(data_name,
//                                                        PmidName(GenerateIdentity()),
//                                                        holders),
//               std::exception);
//  EXPECT_THROW(metadata_handler_.MoveNodeToOnline(data_name, PmidName(GenerateIdentity())),
//               std::exception);

//  // Add element of same name - expect 'corrupt' file to be overwritten
//  EXPECT_NO_THROW(metadata_handler_.AddDataElement(data_name,
//                                                        RandomInt32(),
//                                                        PmidName(GenerateIdentity()),
//                                                        PmidName(GenerateIdentity())));
//  EXPECT_NO_THROW(metadata_handler_.AddDataHolder(data_name, PmidName(GenerateIdentity())));
//  EXPECT_NO_THROW(metadata_handler_.AddOfflinePmid(data_name, PmidName(GenerateIdentity())));
//  EXPECT_NO_THROW(metadata_handler_.RemoveOnlinePmid(data_name, PmidName(GenerateIdentity())));
//  EXPECT_NO_THROW(metadata_handler_.RemoveOfflinePmid(data_name,
//                                                           PmidName(GenerateIdentity())));
//  EXPECT_NO_THROW(metadata_handler_.MoveNodeToOffline(data_name,
//                                                           PmidName(GenerateIdentity()),
//                                                           holders));
//  EXPECT_NO_THROW(metadata_handler_.MoveNodeToOnline(data_name, PmidName(GenerateIdentity())));
//}

//TEST_F(MetadataHandlerTest, DISABLED_BEH_BadProtobuf) {
//  // TODO(Alison) - This test currently places a file in memory, with one of the entries having
//  // an allowed but incorrect value - e.g. the number of copies should be non-negative, and Pmid
//  // names should be of length 64. Currently the test expects that the 'corrupt' file should
//  // cause functions to thrown.
//  // This test should be updated when it has been decided if this functionality is correct.
//  Identity data_name(GenerateIdentity());

//  for (uint16_t i(0); i < 3; ++i) {
//    protobuf::Metadata element;
//    element.set_data_name(data_name.string());
//    element.set_size(RandomInt32());
//    if (i == 0) {  // negative number stored
//      element.set_subscribers(-1);
//      element.add_online_pmid_name(RandomAlphaNumericString(64));
//      element.add_offline_pmid_name(RandomAlphaNumericString(64));
//    } else if (i == 1) {  // too small online pmid name
//      element.set_subscribers(1);
//      element.add_online_pmid_name(RandomAlphaNumericString(RandomUint32() % 64));
//      element.add_offline_pmid_name(RandomAlphaNumericString(64));
//    } else {  // too big offline pmid name
//      element.set_subscribers(1);
//      element.add_online_pmid_name(RandomAlphaNumericString(64));
//      element.add_offline_pmid_name(RandomAlphaNumericString(65 + RandomUint32() % 100));
//    }
//    std::string serialised_element(element.SerializeAsString());
//    EXPECT_FALSE(serialised_element.empty());
//    WriteFile(vault_metadata_dir_ / EncodeToBase64(data_name.string()), serialised_element);

//    EXPECT_THROW(metadata_handler_.AddDataHolder(data_name, PmidName(GenerateIdentity())),
//                 std::exception);
//    EXPECT_THROW(metadata_handler_.AddOfflinePmid(data_name, PmidName(GenerateIdentity())),
//                 std::exception);
//    EXPECT_THROW(metadata_handler_.RemoveOnlinePmid(data_name, PmidName(GenerateIdentity())),
//                 std::exception);
//    EXPECT_THROW(metadata_handler_.RemoveOfflinePmid(data_name, PmidName(GenerateIdentity())),
//                 std::exception);
//    int64_t holders;
//    EXPECT_THROW(metadata_handler_.MoveNodeToOffline(data_name,
//                                                          PmidName(GenerateIdentity()),
//                                                          holders),
//                 std::exception);
//    EXPECT_THROW(metadata_handler_.MoveNodeToOnline(data_name, PmidName(GenerateIdentity())),
//                 std::exception);
//  }
//}

//TEST_F(MetadataHandlerTest, BEH_ManyDataElementsExtensive) {
//  // TODO(Alison) - after the API has been finalised, this est can be made more thorough by calling
//  // AddDataElement more than once with the same data_name_. The use of 'subscribers' will need
//  // to be updated accordingly.
//  int64_t subscribers(1);  // currently only have one for each element in this test
//  std::vector<Identity> data_names;
//  std::vector<int32_t> sizes;
//  std::vector<std::set<PmidName>> online_pmid_names, offline_pmid_names;

//  std::vector<std::vector<PmidName>> online_for_removal, offline_for_removal, online_for_moving,
//                                     offline_for_moving;

//  // initial population, and selection of PmidNames for removing/moving later
//  uint16_t max(100);
//  uint16_t threshold(75);
//  for (uint16_t i(0); i < max; ++i) {
//    data_names.push_back(GenerateIdentity());
//    sizes.push_back(RandomInt32());
//    PmidName online_pmid_name(GenerateIdentity());
//    PmidName offline_pmid_name(GenerateIdentity());

//    online_pmid_names.push_back(std::set<PmidName>());
//    online_pmid_names.back().insert(online_pmid_name);
//    offline_pmid_names.push_back(std::set<PmidName>());
//    offline_pmid_names.back().insert(offline_pmid_name);

//    metadata_handler_.AddDataElement(data_names.back(),
//                                          sizes.back(),
//                                          online_pmid_name,
//                                          offline_pmid_name);
//    CheckDataExistenceAndIntegrity(data_names.back(),
//                                   sizes.back(),
//                                   subscribers,
//                                   online_pmid_name,
//                                   offline_pmid_name);

//    if (i < threshold) {
//      online_for_removal.push_back(std::vector<PmidName>());
//      offline_for_removal.push_back(std::vector<PmidName>());
//      online_for_moving.push_back(std::vector<PmidName>());
//      offline_for_moving.push_back(std::vector<PmidName>());

//      int32_t decider(RandomUint32() % 16);
//      if (decider == 0)
//        online_for_removal.back().push_back(online_pmid_name);
//      if (decider == 4)
//        offline_for_removal.back().push_back(offline_pmid_name);
//      if (decider == 8)
//        online_for_removal.back().push_back(online_pmid_name);
//      if (decider == 12)
//        offline_for_removal.back().push_back(offline_pmid_name);
//    }
//  }

//  // Remove/move/add online/offline PmidNames
//  for (uint16_t i(0); i < threshold; ++i) {
//    for (auto pmid_name : online_for_removal.at(i)) {
//      metadata_handler_.RemoveOnlinePmid(data_names.at(i), pmid_name);
//      online_pmid_names.at(i).erase(pmid_name);
//    }
//    for (auto pmid_name : offline_for_removal.at(i)) {
//      metadata_handler_.RemoveOfflinePmid(data_names.at(i), pmid_name);
//      offline_pmid_names.at(i).erase(pmid_name);
//    }
//    for (auto pmid_name : online_for_moving.at(i)) {
//      int64_t holders;
//      metadata_handler_.MoveNodeToOffline(data_names.at(i), pmid_name, holders);
//      online_pmid_names.at(i).erase(pmid_name);
//      offline_pmid_names.at(i).insert(pmid_name);
//    }
//    for (auto pmid_name : offline_for_removal.at(i)) {
//      metadata_handler_.MoveNodeToOnline(data_names.at(i), pmid_name);
//      offline_pmid_names.at(i).erase(pmid_name);
//      online_pmid_names.at(i).insert(pmid_name);
//    }
//    uint32_t decider(RandomUint32() % 3);
//    if (decider == 0) {
//      PmidName pmid_name(GenerateIdentity());
//      metadata_handler_.AddDataHolder(data_names.at(i), pmid_name);
//      online_pmid_names.at(i).insert(pmid_name);
//    }
//    if (decider == 1) {
//      PmidName pmid_name(GenerateIdentity());
//      metadata_handler_.AddOfflinePmid(data_names.at(i), pmid_name);
//      offline_pmid_names.at(i).insert(pmid_name);
//    }
//  }

//  // Remove some data elements
//  for (uint16_t i(threshold); i < max; ++i) {
//    metadata_handler_.RemoveDataElement(data_names.at(i));
//  }
//  data_names.resize(threshold);
//  sizes.resize(threshold);
//  online_pmid_names.resize(threshold);
//  offline_pmid_names.resize(threshold);

//  // Add some data elements
//  for (uint16_t i(0); i < max - threshold; ++i) {
//    data_names.push_back(GenerateIdentity());
//    sizes.push_back(RandomInt32());
//    PmidName online_pmid_name(GenerateIdentity());
//    PmidName offline_pmid_name(GenerateIdentity());

//    online_pmid_names.push_back(std::set<PmidName>());
//    online_pmid_names.back().insert(online_pmid_name);
//    offline_pmid_names.push_back(std::set<PmidName>());
//    offline_pmid_names.back().insert(offline_pmid_name);

//    metadata_handler_.AddDataElement(data_names.back(),
//                                          sizes.back(),
//                                          online_pmid_name,
//                                          offline_pmid_name);
//  }

//  // TODO(Alison) - Add some data elements that have already been added (see note at top of test)

//  // check data existence and integrity
//  for (uint16_t i(0); i < max; ++i) {
//    CheckDataExistenceAndIntegrity(data_names.at(i),
//                                   sizes.at(i),
//                                   subscribers,
//                                   online_pmid_names.at(i),
//                                   offline_pmid_names.at(i));
//  }
//}


//TEST_F(MetadataHandlerTest, DISABLED_BEH_ManyDataElementsThreaded) {
//  // TODO(Alison) - once the API and behaviour of MetadataHandler has been confirmed, this test
//  // can be written by copying BEH_ManyDataElementsExtensive, and changing the operations involving
//  // metadata_handler_ to be ran in threads.
//}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
