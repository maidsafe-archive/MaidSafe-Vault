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

#include "maidsafe/vault/disk_based_storage.h"

#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/message.h"

#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

template <typename T>
class DiskStorageTest : public testing::Test {
 public:
  DiskStorageTest()
      : root_directory_(maidsafe::test::CreateTestPath("MaidSafe_Test_DiskStorage")),
        disk_based_storage_(*root_directory_),
        element_names_() {}

 protected:
  maidsafe::test::TestPath root_directory_;
  DiskBasedStorage disk_based_storage_;
  std::vector<std::string> element_names_;
  enum Operations { kStore = 0, kDelete, kGetFile, kPutFile };

  std::future<void> StoreAnElement(std::map<typename T::name_type, int32_t>& names) {
    typename T::name_type name((Identity(RandomString(crypto::SHA512::DIGESTSIZE))));
    int32_t value(RandomInt32());
    names.insert(std::make_pair(name, value));
    return disk_based_storage_.Store<T>(name, value);
  }

  std::future<void> StoreAnElement() {
    element_names_.push_back(RandomString(crypto::SHA512::DIGESTSIZE));
    typename T::name_type name((Identity(element_names_.back())));
    int32_t value(RandomInt32());
    return disk_based_storage_.Store<T>(name, value);
  }

  void RunStoreAndDeleteTest(const int kTestEntryCount) {
    int min_files(500), max_files(1000);
    detail::Parameters::set_file_element_count_limits(max_files, min_files);

    // Store kTestEntryCount instances
    std::map<typename T::name_type, int32_t> names;
    for (int n(0); n < kTestEntryCount; ++n)
      EXPECT_NO_THROW(StoreAnElement(names).get());

    // We expect file count = kTestEntryCount / max_files
    EXPECT_EQ(static_cast<size_t>(std::ceil(double(kTestEntryCount) / double(max_files))),
              this->disk_based_storage_.GetFileNames().get().size());

    // Delete all. All should be there.
    for (auto& item : names)
      EXPECT_EQ(item.second, disk_based_storage_.Delete<T>(item.first).get());
  }

  std::string GenerateFileContent(bool store_generated_elements = false) {
    protobuf::DiskStoredFile disk_file;
    for (int n(0); n < detail::Parameters::max_file_element_count(); ++n) {
      protobuf::DiskStoredElement disk_element;
      disk_element.set_name(RandomString(64));
      disk_element.set_value(RandomInt32());
      if (store_generated_elements)
        element_names_.push_back(disk_element.name());

      disk_file.add_element()->CopyFrom(disk_element);
    }
    return disk_file.SerializeAsString();
  }

  std::vector<boost::filesystem::path> VerifyFiles(uint32_t expected_file_num) {
    std::future<uint32_t> file_count(disk_based_storage_.GetFileCount());
    EXPECT_EQ(expected_file_num, file_count.get());
    auto result_get_file_paths = disk_based_storage_.GetFileNames();
    return result_get_file_paths.get();
  }

  void DeleteRandomElement() {
    auto it(element_names_.begin());
    std::advance(it, std::rand() % element_names_.size());
    typename T::name_type name((Identity(*it)));
    auto future(disk_based_storage_.Delete<T>(name));
    future.get();
  }

  void GetRandomFile() {
    std::vector<boost::filesystem::path> names(disk_based_storage_.GetFileNames().get());
    auto it(names.begin());
    std::advance(it, std::rand() % names.size());
    auto future(disk_based_storage_.GetFile(*it));
    future.get();
  }

  void RunOperation(Operations op) {
    switch(op) {
      case kStore: return StoreAnElement().get();
      case kDelete: return DeleteRandomElement();
      case kGetFile: return GetRandomFile();
      default: break;
    }
  }
};

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
                       passport::PublicMpid> AllTypes;

TYPED_TEST_CASE(DiskStorageTest, AllTypes);

TYPED_TEST(DiskStorageTest, BEH_ConstructorDestructor) {
  // Successful construction
  fs::path root_path(*(this->root_directory_) / RandomString(6));
  boost::system::error_code error_code;
  EXPECT_FALSE(fs::exists(root_path, error_code));
  {
    DiskBasedStorage disk_based_storage(root_path);
    EXPECT_TRUE(fs::exists(root_path, error_code));
    EXPECT_EQ(0, disk_based_storage.GetFileNames().get().size());
  }
  EXPECT_TRUE(fs::exists(root_path, error_code));

  // Create a file and give that path as root
  fs::path file_path(*(this->root_directory_) / "a_file");
  ASSERT_TRUE(WriteFile(file_path, RandomString(64)));
  EXPECT_FALSE(fs::exists(root_path, error_code));
  try {
    DiskBasedStorage disk_based_storage(file_path);
    FAIL() << "Object construction should throw with wrong path";
  }
  catch(const std::exception&) {}

  // Give a path that is a non-empty dir with a file that doesn't contain disk based elements
  fs::path dir_path(*(this->root_directory_) / RandomString(6));
  ASSERT_TRUE(WriteFile(dir_path / "file_path", RandomString(64)));
  EXPECT_FALSE(fs::exists(root_path, error_code));
  try {
    DiskBasedStorage disk_based_storage(dir_path);
    FAIL() << "Object construction should throw with wrong path";
  }
  catch(const std::exception&) {}
}

TEST(StandAloneDiskStorageTest, BEH_StoreMultipleDataTypes) {
//  maidsafe::test::TestPath test_directory(maidsafe::test::CreateTestPath());
//  DiskBasedStorage disk_based_storage(*test_directory);
//  const int kTestEntryCount(10000);

}

TYPED_TEST(DiskStorageTest, BEH_SmallStoreAndDelete) {
  const int kTestEntryCount(10000);
  this->RunStoreAndDeleteTest(kTestEntryCount);
}

TYPED_TEST(DiskStorageTest, FUNC_LargeStoreAndDelete) {
  const int kTestEntryCount(10000000);
  this->RunStoreAndDeleteTest(kTestEntryCount);
}

TYPED_TEST(DiskStorageTest, BEH_FileHandlers) {
  std::map<fs::path, NonEmptyString> files;
  int32_t num_files(100);
  std::vector<int32_t> file_numbers;
  for (int32_t i(0); i < num_files; ++i)
    file_numbers.push_back(i);
  std::random_shuffle(file_numbers.begin(), file_numbers.end());

  std::set<fs::path> file_names;
  for (int32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(this->GenerateFileContent());
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(file_content));
    std::string file_name(std::to_string(file_numbers[i]) + "." + EncodeToBase32(hash));
    fs::path file_path(file_name);
    file_names.insert(file_path);
    files.insert(std::make_pair(file_path, file_content));
    this->disk_based_storage_.PutFile(file_path, file_content);
  }

  EXPECT_EQ(this->VerifyFiles(num_files).size(), num_files);
  std::vector<fs::path> retrieved_names(this->disk_based_storage_.GetFileNames().get());
  EXPECT_EQ(file_names.size(), retrieved_names.size());
  for (auto& file_name : retrieved_names)
    EXPECT_FALSE(file_names.find(file_name) == file_names.end());

  boost::system::error_code error_code;
  for (auto& file_entry : files) {
    fs::path path((*this->root_directory_) / file_entry.first);
    EXPECT_TRUE(fs::exists(path, error_code));
    EXPECT_EQ(file_entry.second, this->disk_based_storage_.GetFile(file_entry.first).get());
  }
}

TYPED_TEST(DiskStorageTest, BEH_ActionsWithCorruption) {
  // Setup
  int32_t num_files(20);
  for (int32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(this->GenerateFileContent(true));
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(file_content));
    std::string file_name(std::to_string(i) + "." + EncodeToBase32(hash));
    this->disk_based_storage_.PutFile(file_name, file_content);
  }
  std::vector<typename DiskStorageTest<TypeParam>::Operations> operations;

  // Start operations
  auto future(std::async(std::launch::async,
                         [this, &operations] () {
                           while (!operations.empty()) {
                             try {
                               auto it(operations.begin());
                               this->RunOperation(*it);
                               operations.erase(it);
                             }
                             catch(const std::exception& e) {
                               LOG(kError) << e.what();
                               return;
                             }
                           }
                         }));

  // Corrupt directory

  // Wait for operations
  future.get();
}

//TYPED_TEST(DiskStorageTest, BEH_FileHandlersWithCorruptingThread) {
//  // File handlers of DiskBasedStorage are non-blocking, using a separate Active object
//  Active active;
//  fs::path root_path(*(this->root_directory_) / RandomString(6));
//  DiskBasedStorage disk_based_storage(root_path);
//  std::map<fs::path, NonEmptyString> files;
//  uint32_t num_files(10), max_file_size(10000);
//  for (uint32_t i(0); i < num_files; ++i) {
//    NonEmptyString file_content(this->GenerateFileContent(max_file_size));
//    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(file_content));
//    std::string file_name(std::to_string(i) + "." + EncodeToBase32(hash));
//    fs::path file_path(root_path / file_name);
//    files.insert(std::make_pair(file_path, file_content));
//  }

//  for (auto itr(files.begin()); itr != files.end(); ++itr) {
//    fs::path file_path((*itr).first);
//    active.Send([file_path] () {
//                  maidsafe::WriteFile(file_path, RandomString(100));
//                });
//    disk_based_storage.PutFile(file_path, (*itr).second);
//  }

//  EXPECT_EQ(this->VerifyFiles(num_files, disk_based_storage).size(), num_files);

////   Active active_delete;
//  boost::system::error_code error_code;
//  auto itr = files.begin();
//  do {
//    fs::path path((*itr).first);
//    EXPECT_TRUE(fs::exists(path, error_code));

//    active.Send([&disk_based_storage, path] () {
//                  auto result = disk_based_storage.GetFile(path);
//                  do {
//                    Sleep(boost::posix_time::milliseconds(1));
//                  } while (!result.valid());
//                  EXPECT_FALSE(result.has_exception()) << "Get exception when trying to get "
//                                                       << path.filename();
//                  if (!result.has_exception()) {
//                    NonEmptyString content(result.get());
//                    EXPECT_TRUE(content.IsInitialised());
//                  }
//                });
////     active_delete.Send([path] () { fs::remove(path); });

//    ++itr;
//  } while (itr != files.end());
//}

//TYPED_TEST(DiskStorageTest, BEH_FileMerging) {
//  // Testing with max = 10, min = 5
//  detail::Parameters::set_file_element_count_limits(10, 5);
//  std::shared_ptr<DiskBasedStorage> disk_based_storage;
//  ElementMap element_list;

//  // This bit can probably be made a function to reset the containers and do another case
//  while (element_list.size() != 50U) {
//    disk_based_storage = std::make_shared<DiskBasedStorage>(*this->root_directory_);
//    element_list.clear();
//    for (uint32_t i(0); i < 50U; ++i)
//      StoreAnElement<TypeParam>(100U, *disk_based_storage, element_list);
//  }

//  // Do a trivial case, where only one file goes down to 4

//  // Delete elements so that we have (file index - element count) that tests an interesting scenario
//  // For example: 0 - 8, 1 - 7, 2 - 5, 3 - 9, 4 - 10. Then, delete an element from index 2.
//  // The resulting file scheme should be: 0 - 10,  1 - 10,  2 - 10, 3 - 8


//  // Another interesting scenario: 0 - 8, 1 - 7, 2 - 5, 3 - 5, 4 - 5.
//  // Then, delete an element from index 2.
//  // The resulting file scheme should be: 0 - 10,  1 - 10,  2 - 9
//}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

