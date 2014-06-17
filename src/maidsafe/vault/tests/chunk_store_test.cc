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

const uint64_t kDefaultMaxDiskUsage(4 * 1024);
const uint64_t OneKB(1024);

class ChunkStoreTest {
 public:
  typedef ChunkStore::KeyType KeyType;
  typedef std::vector<std::pair<KeyType, NonEmptyString>> KeyValueContainer;

  struct GenerateKeyValuePair : public boost::static_visitor<NonEmptyString> {
    GenerateKeyValuePair() : size_(OneKB) {}
    explicit GenerateKeyValuePair(uint32_t size) : size_(size) {}

    template <typename T>
    NonEmptyString operator()(T& key) {
      NonEmptyString value = NonEmptyString(RandomAlphaNumericString(size_));
      key.value = Identity(crypto::Hash<crypto::SHA512>(value));
      return value;
    }

    uint32_t size_;
  };

  struct GetIdentity : public boost::static_visitor<Identity> {
    template <typename T>
    Identity operator()(T& key) {
      return key.data;
    }
  };

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
    }
    catch (const std::exception& e) {
      LOG(kError) << e.what();
      return false;
    }
    return true;
  }

  KeyValueContainer PopulateChunkStore(uint32_t num_entries, uint32_t disk_entries,
                                           const fs::path& test_path) {
    boost::system::error_code error_code;
    chunk_store_path_ = test_path;
    KeyValueContainer key_value_pairs;
    NonEmptyString value, recovered;
    KeyType key;

    if (!fs::exists(test_path))
      REQUIRE(fs::create_directories(chunk_store_path_, error_code));
    REQUIRE(0 == error_code.value());
    REQUIRE(fs::exists(chunk_store_path_, error_code));
    REQUIRE(0 == error_code.value());

    AddRandomKeyValuePairs(key_value_pairs, num_entries, OneKB);

    DiskUsage disk_usage(disk_entries * OneKB);
    chunk_store_.reset(new ChunkStore(chunk_store_path_, disk_usage));
    for (auto key_value : key_value_pairs) {
      REQUIRE_NOTHROW(chunk_store_->Put(key_value.first, key_value.second));
      REQUIRE_NOTHROW(recovered = chunk_store_->Get(key_value.first));
      REQUIRE(key_value.second == recovered);
    }
    return key_value_pairs;
  }

  NonEmptyString GenerateKeyValueData(KeyType& key, uint32_t size) {
    GenerateKeyValuePair generate_key_value_pair_(size);
    return boost::apply_visitor(generate_key_value_pair_, key);
  }

  void PrintResult(const pt::ptime& start_time, const pt::ptime& stop_time) {
    uint64_t duration = (stop_time - start_time).total_microseconds();
    if (duration == 0)
      duration = 1;
    std::cout << "Operation completed in " << duration / 1000000.0 << " secs." << std::endl;
  }

  maidsafe::test::TestPath test_path;
  fs::path chunk_store_path_;
  DiskUsage max_disk_usage_;
  std::unique_ptr<ChunkStore> chunk_store_;
};

TEST_CASE_METHOD(ChunkStoreTest, "Constructor", "[Behavioural]") {
  REQUIRE_NOTHROW(ChunkStore(chunk_store_path_, DiskUsage(0)));
  REQUIRE_NOTHROW(ChunkStore(chunk_store_path_, DiskUsage(1)));
  REQUIRE_NOTHROW(ChunkStore(chunk_store_path_, DiskUsage(200000)));
  // Create a path to a file, and check that this can't be used as the disk store path.
  maidsafe::test::TestPath test_path(
      maidsafe::test::CreateTestPath("MaidSafe_Test_ChunkStore"));
  REQUIRE_FALSE(test_path->empty());
  boost::filesystem::path file_path(*test_path / "File");
  REQUIRE(WriteFile(file_path, " "));
  REQUIRE_THROWS_AS(ChunkStore(file_path, DiskUsage(200000)), std::exception);
  REQUIRE_THROWS_AS(ChunkStore(file_path / "base", DiskUsage(200000)), std::exception);
  boost::filesystem::path directory_path(*test_path / "Directory");
  REQUIRE_NOTHROW(ChunkStore(directory_path, DiskUsage(1)));
  REQUIRE(fs::exists(directory_path));
}

TEST_CASE_METHOD(ChunkStoreTest, "RemoveDiskStore", "[Behavioural]") {
  boost::system::error_code error_code;
  maidsafe::test::TestPath test_path(
      maidsafe::test::CreateTestPath("MaidSafe_Test_ChunkStore"));
  fs::path chunk_store_path(*test_path / "new_permanent_store");
  const uintmax_t kSize(1), kDiskSize(2);
  chunk_store_.reset(new ChunkStore(chunk_store_path, DiskUsage(kDiskSize)));
  KeyType key(GetRandomDataNameType());
  NonEmptyString small_value = GenerateKeyValueData(key, kSize);
  REQUIRE_NOTHROW(chunk_store_->Put(key, small_value));
  REQUIRE_NOTHROW(chunk_store_->Delete(key));
  REQUIRE(6 == fs::remove_all(chunk_store_path, error_code));
  REQUIRE_FALSE(fs::exists(chunk_store_path, error_code));
  KeyType key1(GetRandomDataNameType());
  NonEmptyString large_value = GenerateKeyValueData(key1, kDiskSize);
  REQUIRE_THROWS_AS(chunk_store_->Put(key, small_value), std::exception);
  REQUIRE_THROWS_AS(chunk_store_->Get(key), std::exception);
  REQUIRE_THROWS_AS(chunk_store_->Delete(key), std::exception);
  chunk_store_.reset(new ChunkStore(chunk_store_path, DiskUsage(kDiskSize)));
  REQUIRE_NOTHROW(chunk_store_->Put(key1, large_value));
  REQUIRE_NOTHROW(chunk_store_->Delete(key1));
  REQUIRE(6 != fs::remove_all(chunk_store_path, error_code));
  REQUIRE_FALSE(fs::exists(chunk_store_path, error_code));
  REQUIRE_THROWS_AS(chunk_store_->Put(key, small_value), std::exception);
  REQUIRE_THROWS_AS(chunk_store_->Get(key), std::exception);
  REQUIRE_THROWS_AS(chunk_store_->Delete(key), std::exception);
}

TEST_CASE_METHOD(ChunkStoreTest, "SuccessfulStore", "[Behavioural]") {
  KeyType key1(GetRandomDataNameType()), key2(GetRandomDataNameType());
  NonEmptyString value1 = GenerateKeyValueData(key1, static_cast<uint32_t>(2 * OneKB)),
                 value2 = GenerateKeyValueData(key2, static_cast<uint32_t>(2 * OneKB)), recovered;
  REQUIRE_NOTHROW(chunk_store_->Put(key1, value1));
  REQUIRE_NOTHROW(chunk_store_->Put(key2, value2));
  REQUIRE_NOTHROW(recovered = chunk_store_->Get(key1));
  REQUIRE(recovered == value1);
  REQUIRE_NOTHROW(recovered = chunk_store_->Get(key2));
  REQUIRE(recovered == value2);
}

TEST_CASE_METHOD(ChunkStoreTest, "UnsuccessfulStore", "[Behavioural]") {
  KeyType key(GetRandomDataNameType());
  NonEmptyString value = GenerateKeyValueData(key, static_cast<uint32_t>(kDefaultMaxDiskUsage) + 1);
  REQUIRE_THROWS_AS(chunk_store_->Put(key, value), std::exception);
}

TEST_CASE_METHOD(ChunkStoreTest, "DeleteOnDiskStoreOverfill", "[Behavioural]") {
  const size_t num_entries(4), num_disk_entries(4);
  KeyValueContainer key_value_pairs(
      PopulateChunkStore(num_entries, num_disk_entries, chunk_store_path_));
  KeyType key(GetRandomDataNameType());
  NonEmptyString value = GenerateKeyValueData(key, 2 * OneKB), recovered;
  KeyType first_key(key_value_pairs[0].first), second_key(key_value_pairs[1].first);
  REQUIRE_THROWS_AS(chunk_store_->Put(key, value), std::exception);
  REQUIRE_THROWS_AS(recovered = chunk_store_->Get(key), std::exception);
  REQUIRE_NOTHROW(chunk_store_->Delete(first_key));
  REQUIRE_NOTHROW(chunk_store_->Delete(second_key));
  REQUIRE_NOTHROW(chunk_store_->Put(key, value));
  REQUIRE_NOTHROW(recovered = chunk_store_->Get(key));
  REQUIRE(recovered == value);
}

TEST_CASE_METHOD(ChunkStoreTest, "RepeatedlyStoreUsingSameKey", "[Behavioural]") {
  KeyType key(GetRandomDataNameType());
  NonEmptyString value = GenerateKeyValueData(key, (RandomUint32() % 30) + 1), recovered,
                 last_value;
  REQUIRE_NOTHROW(chunk_store_->Put(key, value));
  REQUIRE_NOTHROW(recovered = chunk_store_->Get(key));
  REQUIRE(recovered == value);

  uint32_t events(RandomUint32() % 100);
  for (uint32_t i = 0; i != events; ++i) {
    last_value = NonEmptyString(RandomAlphaNumericString((RandomUint32() % 30) + 1));
    REQUIRE_NOTHROW(chunk_store_->Put(key, last_value));
  }
  REQUIRE_NOTHROW(recovered = chunk_store_->Get(key));
  REQUIRE(value != recovered);
  REQUIRE(last_value == recovered);
  REQUIRE(last_value.string().size() == chunk_store_->GetCurrentDiskUsage().data);
}

TEST_CASE_METHOD(ChunkStoreTest, "Restart", "[Functional]") {
  const size_t num_entries(10 * OneKB), disk_entries(1000 * OneKB);
  KeyValueContainer key_value_pairs(
      PopulateChunkStore(num_entries, disk_entries, chunk_store_path_));
  DiskUsage disk_usage(1000 * OneKB * OneKB);
  std::cout << "Resetting permanent store..." << std::endl;
  pt::ptime start_time(pt::microsec_clock::universal_time());
  chunk_store_.reset(new ChunkStore(chunk_store_path_, disk_usage));
  pt::ptime stop_time(pt::microsec_clock::universal_time());
  PrintResult(start_time, stop_time);
  REQUIRE((num_entries * OneKB) == chunk_store_->GetCurrentDiskUsage().data);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
