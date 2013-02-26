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

#include "maidsafe/common/crypto.h"
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

namespace {

typedef std::future<void> VoidFuture;
typedef std::future<int32_t> Int32Future;
const int kIdSize(crypto::SHA512::DIGESTSIZE);

std::vector<VoidFuture> StoreDataNames(int name_count,
                                       DiskBasedStorage& disk_based_storage,
                                       std::vector<Identity>& data_names,
                                       std::vector<int32_t>& values) {
  std::vector<VoidFuture> futures;
  for (int n(0); n < name_count; ++n) {
    Identity id(RandomString(crypto::SHA512::DIGESTSIZE));
    data_names.push_back(id);
    int32_t value(std::abs(RandomInt32()));
    values.push_back(value);
    switch(n % 15) {
      case 0: futures.push_back(disk_based_storage.Store<passport::PublicAnmid>(
                                    passport::PublicAnmid::name_type(id), value));
              break;
      case 1: futures.push_back(disk_based_storage.Store<passport::PublicAnsmid>(
                                    passport::PublicAnsmid::name_type(id), value));
              break;
      case 2: futures.push_back(disk_based_storage.Store<passport::PublicAntmid>(
                                    passport::PublicAntmid::name_type(id), value));
              break;
      case 3: futures.push_back(disk_based_storage.Store<passport::PublicAnmaid>(
                                    passport::PublicAnmaid::name_type(id), value));
              break;
      case 4: futures.push_back(disk_based_storage.Store<passport::PublicMaid>(
                                    passport::PublicMaid::name_type(id), value));
              break;
      case 5: futures.push_back(disk_based_storage.Store<passport::PublicPmid>(
                                    passport::PublicPmid::name_type(id), value));
              break;
      case 6: futures.push_back(disk_based_storage.Store<passport::Mid>(
                                    passport::Mid::name_type(id), value));
              break;
      case 7: futures.push_back(disk_based_storage.Store<passport::Smid>(
                                    passport::Smid::name_type(id), value));
              break;
      case 8: futures.push_back(disk_based_storage.Store<passport::Tmid>(
                                    passport::Tmid::name_type(id), value));
              break;
      case 9: futures.push_back(disk_based_storage.Store<passport::PublicAnmpid>(
                                    passport::PublicAnmpid::name_type(id), value));
              break;
      case 10: futures.push_back(disk_based_storage.Store<passport::PublicMpid>(
                                     passport::PublicMpid::name_type(id), value));
               break;
      case 11: futures.push_back(disk_based_storage.Store<ImmutableData>(
                                     ImmutableData::name_type(id), value));
               break;
      case 12: futures.push_back(disk_based_storage.Store<OwnerDirectory>(
                                     OwnerDirectory::name_type(id), value));
               break;
      case 13: futures.push_back(disk_based_storage.Store<GroupDirectory>(
                                     GroupDirectory::name_type(id), value));
               break;
      case 14: futures.push_back(disk_based_storage.Store<WorldDirectory>(
                                     WorldDirectory::name_type(id), value));
               break;
    }
  }
  return futures;
}

std::vector<Int32Future> DeleteDataNames(const std::vector<Identity>& data_names,
                                         DiskBasedStorage& disk_based_storage) {
  std::vector<Int32Future> futures;
  for (size_t n(0); n < data_names.size(); ++n) {
    Identity id(data_names.at(n));
    switch(n % 15) {
      case 0: futures.push_back(disk_based_storage.Delete<passport::PublicAnmid>(
                                    passport::PublicAnmid::name_type(id)));
              break;
      case 1: futures.push_back(disk_based_storage.Delete<passport::PublicAnsmid>(
                                    passport::PublicAnsmid::name_type(id)));
              break;
      case 2: futures.push_back(disk_based_storage.Delete<passport::PublicAntmid>(
                                    passport::PublicAntmid::name_type(id)));
              break;
      case 3: futures.push_back(disk_based_storage.Delete<passport::PublicAnmaid>(
                                    passport::PublicAnmaid::name_type(id)));
              break;
      case 4: futures.push_back(disk_based_storage.Delete<passport::PublicMaid>(
                                    passport::PublicMaid::name_type(id)));
              break;
      case 5: futures.push_back(disk_based_storage.Delete<passport::PublicPmid>(
                                    passport::PublicPmid::name_type(id)));
              break;
      case 6: futures.push_back(disk_based_storage.Delete<passport::Mid>(
                                    passport::Mid::name_type(id)));
              break;
      case 7: futures.push_back(disk_based_storage.Delete<passport::Smid>(
                                    passport::Smid::name_type(id)));
              break;
      case 8: futures.push_back(disk_based_storage.Delete<passport::Tmid>(
                                    passport::Tmid::name_type(id)));
              break;
      case 9: futures.push_back(disk_based_storage.Delete<passport::PublicAnmpid>(
                                    passport::PublicAnmpid::name_type(id)));
              break;
      case 10: futures.push_back(disk_based_storage.Delete<passport::PublicMpid>(
                                     passport::PublicMpid::name_type(id)));
               break;
      case 11: futures.push_back(disk_based_storage.Delete<ImmutableData>(
                                     ImmutableData::name_type(id)));
               break;
      case 12: futures.push_back(disk_based_storage.Delete<OwnerDirectory>(
                                     OwnerDirectory::name_type(id)));
               break;
      case 13: futures.push_back(disk_based_storage.Delete<GroupDirectory>(
                                     GroupDirectory::name_type(id)));
               break;
      case 14: futures.push_back(disk_based_storage.Delete<WorldDirectory>(
                                     WorldDirectory::name_type(id)));
               break;
    }
  }
  return futures;
}

std::string GenerateFileContent(std::vector<std::string>* element_names = nullptr) {
  protobuf::DiskStoredFile disk_file;
  for (int n(0); n < detail::Parameters::max_file_element_count(); ++n) {
    protobuf::DiskStoredElement disk_element;
    disk_element.set_name(RandomString(kIdSize));
    disk_element.set_value(std::abs(RandomInt32()));
    if (element_names)
      element_names->push_back(disk_element.name());

    disk_file.add_element()->CopyFrom(disk_element);
  }
  return disk_file.SerializeAsString();
}

std::vector<boost::filesystem::path> VerifyFiles(uint32_t expected_file_num,
                                                 DiskBasedStorage& disk_based_storage) {
  std::future<uint32_t> file_count(disk_based_storage.GetFileCount());
  EXPECT_EQ(expected_file_num, file_count.get());
  auto result_get_file_paths = disk_based_storage.GetFileNames();
  return result_get_file_paths.get();
}

}  // namespace

TEST(StandAloneDiskStorageTest, FUNC_StoreAndDeleteMultipleDataTypes) {
  maidsafe::test::TestPath test_directory(maidsafe::test::CreateTestPath());
  DiskBasedStorage disk_based_storage(*test_directory);
  const int kTestEntryCount(1000000);

  std::vector<Identity> data_names;
  std::vector<int32_t> values;
  std::vector<VoidFuture> store_futures(StoreDataNames(kTestEntryCount,
                                                       disk_based_storage,
                                                       data_names,
                                                       values));
  for (auto& future : store_futures)
    ASSERT_NO_THROW(future.get());

  std::vector<Int32Future> delete_futures(DeleteDataNames(data_names, disk_based_storage));
  for (size_t n(0); n < delete_futures.size(); ++n)
    ASSERT_EQ(values.at(n), delete_futures.at(n).get());
}

TEST(StandAloneDiskStorageTest, BEH_DeleteExceptionCases) {
  maidsafe::test::TestPath test_directory(maidsafe::test::CreateTestPath());
  DiskBasedStorage disk_based_storage(*test_directory);

  ImmutableData::name_type immutable_name((Identity(RandomString(kIdSize))));
  // Throws as no such value exists
  ASSERT_THROW(disk_based_storage.Delete<ImmutableData>(immutable_name).get(), std::exception);

  int32_t value(std::abs(RandomInt32()));
  ASSERT_NO_THROW(disk_based_storage.Store<ImmutableData>(immutable_name, value).get());
  ASSERT_EQ(value, disk_based_storage.Delete<ImmutableData>(immutable_name).get());

  // Throws as value has been deleted
  ASSERT_THROW(disk_based_storage.Delete<ImmutableData>(immutable_name).get(), std::exception);
}

TEST(StandAloneDiskStorageTest, BEH_FileHandlers) {
  maidsafe::test::TestPath test_directory(maidsafe::test::CreateTestPath());
  DiskBasedStorage disk_based_storage(*test_directory);
  std::map<fs::path, NonEmptyString> files;
  int32_t num_files(100);
  std::vector<int32_t> file_numbers;
  for (int32_t i(0); i < num_files; ++i)
    file_numbers.push_back(i);
  std::random_shuffle(file_numbers.begin(), file_numbers.end());

  std::set<fs::path> file_names;
  for (int32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(GenerateFileContent());
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(file_content));
    std::string file_name(std::to_string(file_numbers[i]) + "." + EncodeToBase32(hash));
    fs::path file_path(file_name);
    file_names.insert(file_path);
    files.insert(std::make_pair(file_path, file_content));
    disk_based_storage.PutFile(file_path, file_content);
  }

  EXPECT_EQ(VerifyFiles(num_files, disk_based_storage).size(), num_files);
  std::vector<fs::path> retrieved_names(disk_based_storage.GetFileNames().get());
  EXPECT_EQ(file_names.size(), retrieved_names.size());
  for (auto& file_name : retrieved_names)
    EXPECT_FALSE(file_names.find(file_name) == file_names.end());

  EXPECT_THROW(disk_based_storage.GetFile("non_existent_file").get(), std::exception);
  boost::system::error_code error_code;
  for (auto& file_entry : files) {
    fs::path path((*test_directory) / file_entry.first);
    EXPECT_TRUE(fs::exists(path, error_code));
    EXPECT_EQ(file_entry.second, disk_based_storage.GetFile(file_entry.first).get());
  }
}

TEST(StandAloneDiskStorageTest, BEH_ConstructorDestructor) {
  // Successful construction
  maidsafe::test::TestPath test_directory(maidsafe::test::CreateTestPath());
  fs::path root_path(*(test_directory) / RandomString(6));
  boost::system::error_code error_code;
  EXPECT_FALSE(fs::exists(root_path, error_code));
  {
    DiskBasedStorage disk_based_storage(root_path);
    EXPECT_TRUE(fs::exists(root_path, error_code));
    EXPECT_EQ(0, disk_based_storage.GetFileNames().get().size());
  }
  EXPECT_TRUE(fs::exists(root_path, error_code));

  // Create a file and give that path as root
  fs::path file_path(*(test_directory) / "a_file");
  ASSERT_TRUE(WriteFile(file_path, RandomString(kIdSize)));
  EXPECT_FALSE(fs::exists(root_path, error_code));
  try {
    DiskBasedStorage disk_based_storage(file_path);
    FAIL() << "Object construction should throw with wrong path";
  }
  catch(const std::exception&) {}

  // Give a path that is a non-empty dir with a file that doesn't contain disk based elements
  fs::path dir_path(*(test_directory) / RandomString(6));
  ASSERT_TRUE(WriteFile(dir_path / "file_path", RandomString(kIdSize)));
  EXPECT_FALSE(fs::exists(root_path, error_code));
  try {
    DiskBasedStorage disk_based_storage(dir_path);
    FAIL() << "Object construction should throw with wrong path";
  }
  catch(const std::exception&) {}
}

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
    element_names_.push_back(RandomString(kIdSize));
    typename T::name_type name((Identity(element_names_.back())));
    int32_t value(std::abs(RandomInt32()));
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
    return test::GenerateFileContent(store_generated_elements ? &element_names_ : nullptr);
  }

  std::vector<boost::filesystem::path> VerifyFiles(uint32_t expected_file_num) {
    return VerifyFiles(expected_file_num, disk_based_storage_);
  }

  void DeleteRandomElement() {
    auto it(element_names_.begin());
    std::advance(it, std::rand() % element_names_.size());
    typename T::name_type name((Identity(*it)));
    auto future(disk_based_storage_.Delete<T>(name));
    future.get();
  }

  void GetRandomFile() {
    auto names(disk_based_storage_.GetFileNames().get());
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

  std::vector<Operations> GenerateOperations(int op_count) {
    std::vector<Operations> ops;
    for (int n(0); n < op_count; ++n)
      ops.push_back(static_cast<Operations>(maidsafe::RandomUint32() % 3));
    return std::move(ops);
  }

  void RunCorruptions() {
    int rounds(100);
    boost::system::error_code ec;
    std::string corrupting_string(RandomString(1000));
    while (rounds-- > 0) {
      auto names(disk_based_storage_.GetFileNames().get());
      auto it(names.begin());
      std::advance(it, std::rand() % names.size());
      fs::path file_path(*root_directory_ / *it);
      switch (rounds % 3) {
        case 0: fs::remove(file_path, ec);
                break;
        case 1: WriteFile(file_path, corrupting_string);
                break;
        case 2: fs::rename(file_path, *root_directory_ / RandomAlphaNumericString(6), ec);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
};

TYPED_TEST_CASE_P(DiskStorageTest/*, AllTypes*/);

TYPED_TEST_P(DiskStorageTest, BEH_SmallStoreAndDelete) {
  const int kTestEntryCount(10000);
  this->RunStoreAndDeleteTest(kTestEntryCount);
}

TYPED_TEST_P(DiskStorageTest, FUNC_LargeStoreAndDelete) {
  const int kTestEntryCount(10000000);
  this->RunStoreAndDeleteTest(kTestEntryCount);
}

TYPED_TEST_P(DiskStorageTest, BEH_ActionsWithCorruption) {
  // Setup
  int num_files(20), num_ops(5000);
  for (int32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(this->GenerateFileContent(true));
    crypto::SHA512Hash hash(crypto::Hash<crypto::SHA512>(file_content));
    std::string file_name(std::to_string(i) + "." + EncodeToBase32(hash));
    this->disk_based_storage_.PutFile(file_name, file_content);
  }

  typedef typename DiskStorageTest<TypeParam>::Operations Ops;
  std::vector<Ops> operations(this->GenerateOperations(num_ops));

  // Start operations
  std::atomic_bool curruption_running(false), failed(false);
  auto ops_future(std::async(std::launch::async,
                             [this, &operations, &curruption_running, &failed] () {
                               while (!operations.empty()) {
                                 try {
                                   auto it(operations.begin());
                                   this->RunOperation(*it);
                                   operations.erase(it);
                                 }
                                 catch(const std::exception& e) {
                                   LOG(kError) << e.what();
                                   if (!curruption_running)
                                     failed = true;
                                   return;
                                 }
                               }
                               LOG(kInfo) << "Finished executing operations.";
                             }));

  // Corrupt directory
  auto corrupt_future(std::async(std::launch::async,
                                 [this, &curruption_running] () {
                                   // Allow ops to run for a while. Might need adjustment.
                                   std::this_thread::sleep_for(std::chrono::seconds(7));
                                   curruption_running =  true;
                                   this->RunCorruptions();
                                 }));

  // Wait for operations
  ops_future.get();
  EXPECT_FALSE(failed);
  corrupt_future.get();
}

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

REGISTER_TYPED_TEST_CASE_P(DiskStorageTest,
                           BEH_SmallStoreAndDelete,
                           FUNC_LargeStoreAndDelete,
                           BEH_ActionsWithCorruption);

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

INSTANTIATE_TYPED_TEST_CASE_P(TypeSpecific, DiskStorageTest, AllTypes);


}  // namespace test

}  // namespace vault

}  // namespace maidsafe

