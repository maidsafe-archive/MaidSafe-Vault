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

#include "maidsafe/vault/metadata_manager/data_elements_manager.h"

#include <vector>

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/metadata_manager/metadata_pb.h"


namespace maidsafe {

namespace vault {

namespace test {

// to match kVaultDirectory in data_elements_manager.cc
const boost::filesystem::path kVaultDirectory("meta_data_manager");

Identity GenerateIdentity() {
  return Identity(RandomAlphaNumericString(64));
}

class DataElementsManagerTest : public testing::Test {
 public:
  DataElementsManagerTest()
    : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_DataElementsManager")),
      vault_root_dir_(*kTestRoot_ / RandomAlphaNumericString(8)),
      vault_metadata_dir_(vault_root_dir_ / kVaultDirectory),
      data_elements_manager_(vault_root_dir_) {}

 private:
  bool CheckDataExistenceAndParsing(const Identity& data_name,
                                    protobuf::MetadataElement& element) {
    if (!boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name))) {
      LOG(kError) << "Data was not found.";
      return false;
    }
    NonEmptyString serialised_element(ReadFile(vault_metadata_dir_ / EncodeToBase64(data_name)));
    if (!element.ParseFromString(serialised_element.string())) {
      LOG(kError) << "Data did not parse.";
      return false;
    }
    return true;
  }

  bool CheckPmidIds(protobuf::MetadataElement& element,
                    const std::set<PmidName>& online_pmids,
                    const std::set<PmidName>& offline_pmids) {
    std::set<PmidName> element_online_pmid_names;
    for (uint16_t i(0); i < element.online_pmid_name_size(); ++i) {
      try {
        PmidName online_pmid_name(Identity(element.online_pmid_name(i)));
        element_online_pmid_names.insert(online_pmid_name);
      } catch(const std::exception& ex) {
        LOG(kError) << "Data has online pmid of wrong size.  " << ex.what();
        return false;
      }
    }

    if (online_pmids != element_online_pmid_names) {
      LOG(kError) << "Data's online pmid IDs don't match expectation.";
      LOG(kVerbose) << "\n\nonline - expected:";
      for (auto pmid : online_pmids)
        LOG(kVerbose) << pmid->string();
      LOG(kVerbose) << "\nonline - actual:";
      for (auto pmid : element_online_pmid_names)
        LOG(kVerbose) << pmid->string();
      return false;
    }

    std::set<PmidName> element_offline_pmid_names;
    for (uint16_t i(0); i < element.offline_pmid_name_size(); ++i) {
      try {
        PmidName offline_pmid_name(Identity(element.offline_pmid_name(i)));
        element_offline_pmid_names.insert(offline_pmid_name);
      } catch(const std::exception& ex) {
        LOG(kError) << "Data has offline pmid of wrong size.  " << ex.what();
        return false;
      }
    }
    if (offline_pmids != element_offline_pmid_names) {
      LOG(kError) << "Data's offline pmid IDs don't match expectation.";
      LOG(kVerbose) << "\n\noffline - expected:";
      for (auto pmid : offline_pmids)
        LOG(kVerbose) << pmid->string();
      LOG(kVerbose) << "\noffline - actual:";
      for (auto pmid : element_offline_pmid_names)
        LOG(kVerbose) << pmid->string();
      return false;
    }

    return true;
  }

  bool CheckPmidIds(protobuf::MetadataElement& element,
                    const PmidName& online_pmid,
                    const PmidName& offline_pmid) {
    if (element.online_pmid_name_size() != 1) {
      LOG(kError) << "Expected one online pmid, found " << element.online_pmid_name_size();
      return false;
    }

    if (element.offline_pmid_name_size() != 1) {
      LOG(kError) << "Expected one offline pmid, found " << element.offline_pmid_name_size();
      return false;
    }

    if (element.online_pmid_name(0) != online_pmid->string()) {
      LOG(kError) << "Data's online pmid is wrong.";
      return false;
    }

    if (element.offline_pmid_name(0) != offline_pmid->string()) {
      LOG(kError) << "Data's offline pmid is wrong.";
      return false;
    }

    return true;
  }

 public:
  bool CheckDataExistenceAndIntegrity(const Identity& data_name,
                                      const int32_t& element_size,
                                      const std::set<PmidName>& online_pmids,
                                      const std::set<PmidName>& offline_pmids) {
    protobuf::MetadataElement element;
    if (!CheckDataExistenceAndParsing(data_name, element))
      return false;

    if (element.element_size() != element_size) {
      LOG(kError) << "Data has wrong size.";
      return false;
    }

    if (online_pmids.empty() && (element.online_pmid_name_size() != 0)) {
      LOG(kError) << "Data has too many online pmid IDs.";
      return false;
    }
    if (offline_pmids.empty() && (element.offline_pmid_name_size() != 0)) {
      LOG(kError) << "Data has too many offline pmid IDs.";
      return false;
    }

    if (!CheckPmidIds(element, online_pmids, offline_pmids))
      return false;

    return true;
  }

  bool CheckDataExistenceAndIntegrity(const Identity &data_name,
                                      const int32_t &element_size,
                                      const PmidName& online_pmid,
                                      const PmidName& offline_pmid) {
    protobuf::MetadataElement element;
    if (!CheckDataExistenceAndParsing(data_name, element))
      return false;

    if (element.element_size() != element_size) {
      LOG(kError) << "Data has wrong size.";
      return false;
    }

    if (!CheckPmidIds(element, online_pmid, offline_pmid))
      return false;

    return true;
  }

  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  boost::filesystem::path vault_metadata_dir_;
  DataElementsManager data_elements_manager_;
};

class DataElementsManagerOneElementTest : public DataElementsManagerTest {
 public:
  DataElementsManagerOneElementTest()
      : data_name_(GenerateIdentity()),
        element_size_(RandomInt32()),
        online_pmid_name_(GenerateIdentity()),
        offline_pmid_name_(GenerateIdentity()) {}

 protected:
  void SetUp() {
    online_pmid_names_.insert(online_pmid_name_);
    offline_pmid_names_.insert(offline_pmid_name_);
  }

 public:
  Identity data_name_;
  int32_t element_size_;
  PmidName online_pmid_name_;
  PmidName offline_pmid_name_;
  std::set<PmidName> online_pmid_names_;
  std::set<PmidName> offline_pmid_names_;
};

TEST_F(DataElementsManagerOneElementTest, BEH_AddDataElement) {
  // Add and check data element, then re-add it and check again
  for (uint16_t i(0); i < 2; ++i) {
    data_elements_manager_.AddDataElement(data_name_,
                                          element_size_,
                                          online_pmid_name_,
                                          offline_pmid_name_);

    EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));
  }
}

TEST_F(DataElementsManagerOneElementTest, BEH_AddChangedDataElement) {
  // Add and check data element, then add another data element with the same id, but with other
  // fields different, and check that original data element's content hasn't changed.
  for (uint16_t i(0); i < 2; ++i) {
    if (i == 0) {
      data_elements_manager_.AddDataElement(data_name_,
                                            element_size_,
                                            online_pmid_name_,
                                            offline_pmid_name_);
    } else {
      data_elements_manager_.AddDataElement(data_name_,
                                            RandomInt32(),
                                            PmidName(GenerateIdentity()),
                                            PmidName(GenerateIdentity()));
    }

    EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));
  }
}

TEST_F(DataElementsManagerOneElementTest, BEH_AddAndRemoveDataElement) {
  data_elements_manager_.AddDataElement(data_name_, element_size_, online_pmid_name_,
                                        offline_pmid_name_);
  EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                             element_size_,
                                             online_pmid_names_,
                                             offline_pmid_names_));

  data_elements_manager_.RemoveDataElement(data_name_);
  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name_)));

  data_elements_manager_.AddDataElement(data_name_, element_size_, online_pmid_name_,
                                        offline_pmid_name_);
  EXPECT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                             element_size_,
                                             online_pmid_names_,
                                             offline_pmid_names_));
}

TEST_F(DataElementsManagerTest, BEH_RemoveNonexistentDataElement) {
  Identity data_name(GenerateIdentity());

  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));

  // TODO(Alison) - sensitise to NfsErrors::failed_to_find_managed_element?
  EXPECT_THROW(data_elements_manager_.RemoveDataElement(data_name),
               std::exception);

  EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name)));
}

TEST_F(DataElementsManagerOneElementTest, BEH_AddRemovePmids) {
  // TODO(Alison) - tidy/condense this test
  data_elements_manager_.AddDataElement(data_name_, element_size_, online_pmid_name_,
                                        offline_pmid_name_);

  ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                             element_size_,
                                             online_pmid_names_,
                                             offline_pmid_names_));

  // Add, and randomly select some for removal later
  std::vector<PmidName> online_for_removal;
  std::vector<PmidName> offline_for_removal;
  for (uint16_t i(0); i < 20; ++i) {  // TODO(Alison) - max value?
    PmidName online_pmid_name(GenerateIdentity());
    data_elements_manager_.AddOnlinePmid(data_name_, online_pmid_name);
    online_pmid_names_.insert(online_pmid_name);
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));
    if (RandomUint32() % 3 == 0)
      online_for_removal.push_back(online_pmid_name);

    PmidName offline_pmid_name(GenerateIdentity());
    data_elements_manager_.AddOfflinePmid(data_name_, offline_pmid_name);
    offline_pmid_names_.insert(offline_pmid_name);
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));
    if (RandomUint32() % 3 == 0)
      offline_for_removal.push_back(offline_pmid_name);
  }

  // Remove some (try to randomise orders a bit)
  while (!online_for_removal.empty() && !offline_for_removal.empty()) {
    if (!online_for_removal.empty() && (RandomUint32() % 4 != 0)) {
      size_t index(RandomUint32() % online_for_removal.size());
      PmidName to_remove(online_for_removal.at(index));
      data_elements_manager_.RemoveOnlinePmid(data_name_, to_remove);
      online_pmid_names_.erase(to_remove);
      online_for_removal.erase(online_for_removal.begin() + index);
      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                                 element_size_,
                                                 online_pmid_names_,
                                                 offline_pmid_names_));
    }

    if (!offline_for_removal.empty() && (RandomUint32() % 4 != 0)) {
      size_t index(RandomUint32() % offline_for_removal.size());
      PmidName to_remove(offline_for_removal.at(index));
      data_elements_manager_.RemoveOfflinePmid(data_name_, to_remove);
      offline_pmid_names_.erase(to_remove);
      offline_for_removal.erase(offline_for_removal.begin() + index);
      ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                                 element_size_,
                                                 online_pmid_names_,
                                                 offline_pmid_names_));
    }
  }


  // Add some more
  for (uint16_t i(0); i < 20; ++i) {  // TODO(Alison) - max value?
    PmidName online_pmid_name(GenerateIdentity());
    data_elements_manager_.AddOnlinePmid(data_name_, online_pmid_name);
    online_pmid_names_.insert(online_pmid_name);
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));

    PmidName offline_pmid_name(GenerateIdentity());
    data_elements_manager_.AddOfflinePmid(data_name_, offline_pmid_name);
    offline_pmid_names_.insert(offline_pmid_name);
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_name_,
                                               element_size_,
                                               online_pmid_names_,
                                               offline_pmid_names_));
  }
}

TEST_F(DataElementsManagerOneElementTest, BEH_MovePmids) {
  // TODO(Alison)
  // Move Pmids between online and offline. Check that sets are correct and that the number of
  // 'holders' is updated correctly
}

TEST_F(DataElementsManagerTest, BEH_ManyDataElements) {
  // Add and remove many data elements in various combinations
  std::vector<Identity> data_names;
  std::vector<int32_t> element_sizes;
  std::vector<PmidName> online_pmid_names;
  std::vector<PmidName> offline_pmid_names;

  // Add elements
  uint16_t max(1000);
  for (uint16_t i(0); i < max; ++i) {
    data_names.push_back(GenerateIdentity());
    element_sizes.push_back(RandomInt32());
    online_pmid_names.push_back(PmidName(GenerateIdentity()));
    offline_pmid_names.push_back(PmidName(GenerateIdentity()));
    data_elements_manager_.AddDataElement(data_names.back(),
                                          element_sizes.back(),
                                          online_pmid_names.back(),
                                          offline_pmid_names.back());
  }

  // Check existence
  for (uint16_t i(0); i < max; ++i) {
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
                                               element_sizes.at(i),
                                               online_pmid_names.at(i),
                                               offline_pmid_names.at(i)));
  }

  // Remove elements
  for (uint16_t i(0); i < max; i += 2) {
    data_elements_manager_.RemoveDataElement(data_names.at(i));
  }

  // Check non-existence and existence
  for (uint16_t i(0); i < max; i += 2)
    EXPECT_FALSE(boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_names.at(i))));

  for (uint16_t i(1); i < max; i += 2) {
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
                                               element_sizes.at(i),
                                               online_pmid_names.at(i),
                                               offline_pmid_names.at(i)));
  }

  // Re-add removed elements
  for (uint16_t i(0); i < max; i += 2) {
    data_elements_manager_.AddDataElement(data_names.at(i),
                                          element_sizes.at(i),
                                          online_pmid_names.at(i),
                                          offline_pmid_names.at(i));
  }

  // Check existence
  for (uint16_t i(0); i < max; ++i) {
    ASSERT_TRUE(CheckDataExistenceAndIntegrity(data_names.at(i),
                                               element_sizes.at(i),
                                               online_pmid_names.at(i),
                                               offline_pmid_names.at(i)));
  }
}

// TODO(Alison) - test Pmid operations on multiple data elements (cf. 'BEH_ManyDataElements' test)

// TODO(Alison) - add threaded tests

// TODO(Alison) - test that badly formatted protobufs are handled correctly (especially with
// strings of length != 64)

// TODO(Alison) - test private functions?

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
