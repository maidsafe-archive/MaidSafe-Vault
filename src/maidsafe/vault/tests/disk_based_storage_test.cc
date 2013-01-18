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


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

template <typename T>
class DiskStorageTest : public testing::Test {
 public:
  DiskStorageTest()
      : root_directory_(maidsafe::test::CreateTestPath("MaidSafe_Test_DiskStorage")) {}

 protected:
  maidsafe::test::TestPath root_directory_;
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
  fs::path root_path(*(this->root_directory_) / RandomString(6));
  boost::system::error_code error_code;
  EXPECT_FALSE(fs::exists(root_path, error_code));
  {
    DiskBasedStorage disk_based_storage(root_path);
    EXPECT_TRUE(fs::exists(root_path, error_code));
  }
  EXPECT_TRUE(fs::exists(root_path, error_code));
}

TYPED_TEST(DiskStorageTest, BEH_FileHandlers) {
  fs::path root_path(*(this->root_directory_) / RandomString(6));
  DiskBasedStorage disk_based_storage(root_path);

  std::map<fs::path, NonEmptyString> files;
  uint32_t num_files(100), max_file_size(10000);
  for (uint32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(RandomString(RandomUint32() % max_file_size));
    fs::path file_path(root_path / RandomString(num_files));
    files.insert(std::make_pair(file_path, file_content));
    disk_based_storage.WriteFile(file_path, file_content);
  }

  uint32_t file_count(disk_based_storage.GetFileCount());
  EXPECT_EQ(file_count, num_files);
  std::vector<boost::filesystem::path> file_paths = disk_based_storage.GetFileNames();
  EXPECT_EQ(file_paths.size(), num_files);

  boost::system::error_code error_code;
  auto itr = files.begin();
  do {
    fs::path path((*itr).first);
    EXPECT_TRUE(fs::exists(path, error_code));

    auto result = disk_based_storage.GetFile(path);
    NonEmptyString content(result.get());
    EXPECT_EQ(content, (*itr).second);

    ++itr;
  } while (itr != files.end());
}

TYPED_TEST(DiskStorageTest, BEH_FileHandlersWithCorruptingThread) {
  // File handlers of DiskBasedStorage are non-blocking, using a separate Active object
  Active active;
  fs::path root_path(*(this->root_directory_) / RandomString(6));
  DiskBasedStorage disk_based_storage(root_path);

  std::map<fs::path, NonEmptyString> files;
  uint32_t num_files(10), max_file_size(10000);
  for (uint32_t i(0); i < num_files; ++i) {
    NonEmptyString file_content(RandomString(RandomUint32() % max_file_size));
    fs::path file_path(root_path / RandomString(num_files));
    files.insert(std::make_pair(file_path, file_content));
    active.Send([file_path] () {
                  maidsafe::WriteFile(file_path, RandomString(100));
                });
    disk_based_storage.WriteFile(file_path, file_content);
  }

  Active active_delete;
  boost::system::error_code error_code;
  auto itr = files.begin();
  do {
    fs::path path((*itr).first);
    EXPECT_TRUE(fs::exists(path, error_code));

    active.Send([&disk_based_storage, path] () {
                  auto result = disk_based_storage.GetFile(path);
                  NonEmptyString content(result.get());
                  EXPECT_TRUE(content.IsInitialised());
                });
    active_delete.Send([path] () { fs::remove(path); });

    ++itr;
  } while (itr != files.end());
}

TYPED_TEST(DiskStorageTest, BEH_ElementHandlers) {
  fs::path root_path(*(this->root_directory_) / RandomString(6));
  DiskBasedStorage disk_based_storage(root_path);

  std::string file_name(RandomString(crypto::SHA512::DIGESTSIZE));
  typename TypeParam::name_type name((Identity(file_name)));
  int32_t version(RandomUint32());
  std::string serialised_value(RandomString(10000));
  disk_based_storage.Store<TypeParam>(name, version, serialised_value);
  boost::system::error_code error_code;
  EXPECT_TRUE(fs::exists(root_path / file_name, error_code));
  {
    auto result = disk_based_storage.GetFile(root_path / file_name);
    NonEmptyString fetched_content = result.get();
    EXPECT_EQ(fetched_content.string(), serialised_value);
  }

  std::string new_serialised_value(RandomString(10000));
  disk_based_storage.Modify<TypeParam>(name, version, nullptr, new_serialised_value);
  {
    auto result = disk_based_storage.GetFile(root_path / file_name);
    NonEmptyString fetched_content = result.get();
    EXPECT_EQ(fetched_content.string(), new_serialised_value);
  }

  disk_based_storage.Delete<TypeParam>(name, version);
  EXPECT_FALSE(fs::exists(root_path / file_name, error_code));
}

TYPED_TEST(DiskStorageTest, BEH_ElementHandlersWithMultThreads) {
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

