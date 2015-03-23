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

#include "maidsafe/vault/chunk_store.h"

#include <memory>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

#include "maidsafe/common/convert.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/vault/tests/chunk_store_test_utils.h"

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace args = std::placeholders;

namespace maidsafe {

namespace vault {

namespace test {

const std::uint64_t OneKB(1024);
const std::uint64_t AesPadding(16);
// Allow 16 bytes extra per chunk since we're AES encrypting them
const std::uint64_t kDefaultMaxDiskUsage(4 * (OneKB + AesPadding));

class ChunkStoreTest : public testing::Test {
 public:
  typedef ChunkStore::NameType NameType;
  typedef std::vector<std::pair<NameType, NonEmptyString>> NameValueContainer;

 protected:
  ChunkStoreTest()
      : test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_ChunkStore")),
        chunk_store_path_(*test_path / "permanent_store"),
        max_disk_usage_(kDefaultMaxDiskUsage),
        chunk_store_(new ChunkStore(chunk_store_path_, max_disk_usage_)) {}

  bool DeleteDirectory(const fs::path& directory) {
    boost::system::error_code error_code;
    fs::directory_iterator end;
    try {
      fs::directory_iterator it(directory);
      for (; it != end; ++it)
        fs::remove_all((*it).path(), error_code);
      if (error_code)
        return false;
    } catch (const std::exception& e) {
      LOG(kError) << e.what();
      return false;
    }
    return true;
  }

  NameValueContainer PopulateChunkStore(std::uint32_t num_entries, std::uint32_t disk_entries,
                                        const fs::path& test_path) {
    boost::system::error_code error_code;
    chunk_store_path_ = test_path;
    NameValueContainer name_value_pairs;
    NonEmptyString recovered;

    if (!fs::exists(test_path))
      EXPECT_TRUE(fs::create_directories(chunk_store_path_, error_code));
    EXPECT_TRUE(0 == error_code.value());
    EXPECT_TRUE(fs::exists(chunk_store_path_, error_code));
    EXPECT_TRUE(0 == error_code.value());

    AddRandomNameValuePairs(name_value_pairs, num_entries, OneKB);

    DiskUsage disk_usage(disk_entries * (OneKB + AesPadding));
    chunk_store_.reset(new ChunkStore(chunk_store_path_, disk_usage));
    for (auto name_value : name_value_pairs) {
      EXPECT_NO_THROW(chunk_store_->Put(name_value.first, name_value.second));
      EXPECT_NO_THROW(recovered = chunk_store_->Get(name_value.first));
      EXPECT_TRUE(name_value.second == recovered);
    }
    return name_value_pairs;
  }

  void PrintResult(const pt::ptime& start_time, const pt::ptime& stop_time) {
    std::uint64_t duration = (stop_time - start_time).total_microseconds();
    if (duration == 0)
      duration = 1;
    std::cout << "Operation completed in " << duration / 1000000.0 << " secs." << std::endl;
  }

  maidsafe::test::TestPath test_path;
  fs::path chunk_store_path_;
  DiskUsage max_disk_usage_;
  std::unique_ptr<ChunkStore> chunk_store_;
};

TEST_F(ChunkStoreTest, BEH_Constructor) {
  ASSERT_NO_THROW(ChunkStore(chunk_store_path_, DiskUsage(0)));
  ASSERT_NO_THROW(ChunkStore(chunk_store_path_, DiskUsage(1)));
  ASSERT_NO_THROW(ChunkStore(chunk_store_path_, DiskUsage(200000)));
  // Create a path to a file, and check that this can't be used as the disk store path.
  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_ChunkStore"));
  ASSERT_FALSE(test_path->empty());
  boost::filesystem::path file_path(*test_path / "File");
  EXPECT_TRUE(WriteFile(file_path, convert::ToByteVector(" ")));
  ASSERT_THROW(ChunkStore(file_path, DiskUsage(200000)), std::exception);
  ASSERT_THROW(ChunkStore(file_path / "base", DiskUsage(200000)), std::exception);
  boost::filesystem::path directory_path(*test_path / "Directory");
  ASSERT_NO_THROW(ChunkStore(directory_path, DiskUsage(1)));
  EXPECT_TRUE(fs::exists(directory_path));
}

TEST_F(ChunkStoreTest, BEH_RemoveDiskStore) {
  boost::system::error_code error_code;
  maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_ChunkStore"));
  fs::path chunk_store_path(*test_path / "new_permanent_store");
  const std::uintmax_t kSize(1), kDiskSize(116);
  chunk_store_.reset(new ChunkStore(chunk_store_path, DiskUsage(kDiskSize)));
  NameType name(MakeIdentity(), DataTypeId(RandomUint32()));
  NonEmptyString small_value(RandomBytes(kSize));
  ASSERT_NO_THROW(chunk_store_->Put(name, small_value));
  ASSERT_NO_THROW(chunk_store_->Delete(name));
  EXPECT_TRUE(6 == fs::remove_all(chunk_store_path, error_code));
  ASSERT_FALSE(fs::exists(chunk_store_path, error_code));
  NameType name1(MakeIdentity(), DataTypeId(RandomUint32()));
  // The data gets AES encrypted and will end up at most 16 bytes larger when written to the store
  NonEmptyString large_value(RandomBytes(kDiskSize - AesPadding));
  EXPECT_THROW(chunk_store_->Put(name, small_value), std::exception);
  EXPECT_THROW(chunk_store_->Get(name), std::exception);
  EXPECT_THROW(chunk_store_->Delete(name), std::exception);
  chunk_store_.reset(new ChunkStore(chunk_store_path, DiskUsage(kDiskSize)));
  ASSERT_NO_THROW(chunk_store_->Put(name1, large_value));
  ASSERT_NO_THROW(chunk_store_->Delete(name1));
  EXPECT_TRUE(6 != fs::remove_all(chunk_store_path, error_code));
  ASSERT_FALSE(fs::exists(chunk_store_path, error_code));
  EXPECT_THROW(chunk_store_->Put(name, small_value), std::exception);
  EXPECT_THROW(chunk_store_->Get(name), std::exception);
  EXPECT_THROW(chunk_store_->Delete(name), std::exception);
}

TEST_F(ChunkStoreTest, BEH_SuccessfulStore) {
  NameType name1(MakeIdentity(), DataTypeId(RandomUint32()));
  NameType name2(MakeIdentity(), DataTypeId(RandomUint32()));
  NonEmptyString value1(RandomBytes(2 * OneKB)), value2(RandomBytes(2 * OneKB)), recovered;
  ASSERT_NO_THROW(chunk_store_->Put(name1, value1));
  ASSERT_NO_THROW(chunk_store_->Put(name2, value2));
  ASSERT_NO_THROW(recovered = chunk_store_->Get(name1));
  EXPECT_TRUE(recovered == value1);
  ASSERT_NO_THROW(recovered = chunk_store_->Get(name2));
  EXPECT_TRUE(recovered == value2);
}

TEST_F(ChunkStoreTest, BEH_UnsuccessfulStore) {
  NameType name(MakeIdentity(), DataTypeId(RandomUint32()));
  NonEmptyString value(RandomBytes(kDefaultMaxDiskUsage + 1));
  EXPECT_THROW(chunk_store_->Put(name, value), std::exception);
}

TEST_F(ChunkStoreTest, BEH_DeleteOnDiskStoreOverfill) {
  const size_t num_entries(4), num_disk_entries(4);
  NameValueContainer name_value_pairs(
      PopulateChunkStore(num_entries, num_disk_entries, chunk_store_path_));
  NameType name(MakeIdentity(), DataTypeId(RandomUint32()));
  NonEmptyString value(RandomBytes(2 * OneKB)), recovered;
  NameType first_name(name_value_pairs[0].first), second_name(name_value_pairs[1].first);
  EXPECT_THROW(chunk_store_->Put(name, value), std::exception);
  EXPECT_THROW(recovered = chunk_store_->Get(name), std::exception);
  ASSERT_NO_THROW(chunk_store_->Delete(first_name));
  ASSERT_NO_THROW(chunk_store_->Delete(second_name));
  ASSERT_NO_THROW(chunk_store_->Put(name, value));
  ASSERT_NO_THROW(recovered = chunk_store_->Get(name));
  EXPECT_TRUE(recovered == value);
}

TEST_F(ChunkStoreTest, BEH_RepeatedlyStoreUsingSameName) {
  NameType name(MakeIdentity(), DataTypeId(RandomUint32()));
  NonEmptyString value(RandomBytes(1, 30)), recovered, last_value;
  ASSERT_NO_THROW(chunk_store_->Put(name, value));
  ASSERT_NO_THROW(recovered = chunk_store_->Get(name));
  EXPECT_TRUE(recovered == value);

  std::uint32_t events((RandomUint32() % 100) + 1);
  for (std::uint32_t i = 0; i != events; ++i) {
    last_value = NonEmptyString(RandomAlphaNumericBytes(1, 30));
    ASSERT_NO_THROW(chunk_store_->Put(name, last_value));
  }
  ASSERT_NO_THROW(recovered = chunk_store_->Get(name));
  EXPECT_TRUE(value != recovered);
  EXPECT_TRUE(last_value == recovered);
  EXPECT_EQ(last_value.string().size() + AesPadding, chunk_store_->CurrentDiskUsage().data);
}

TEST_F(ChunkStoreTest, FUNC_Restart) {
  const size_t num_entries(10 * OneKB), disk_entries(1000 * OneKB);
  NameValueContainer name_value_pairs(
      PopulateChunkStore(num_entries, disk_entries, chunk_store_path_));
  DiskUsage disk_usage(1000 * OneKB * OneKB);
  std::cout << "Resetting permanent store..." << std::endl;
  pt::ptime start_time(pt::microsec_clock::universal_time());
  chunk_store_.reset(new ChunkStore(chunk_store_path_, disk_usage));
  pt::ptime stop_time(pt::microsec_clock::universal_time());
  PrintResult(start_time, stop_time);
  EXPECT_EQ((num_entries * (OneKB + AesPadding)), chunk_store_->CurrentDiskUsage().data);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
