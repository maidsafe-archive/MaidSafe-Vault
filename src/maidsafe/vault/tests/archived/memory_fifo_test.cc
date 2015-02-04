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

#include "maidsafe/vault/memory_fifo.h"

#include <memory>

#include "maidsafe/common/error.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/vault/tests/chunk_store_test_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

const uint64_t kDefaultMaxMemoryUsage(10);  // elements
const uint64_t OneKB(1024);

class MemoryFIFOTest : public testing::Test {
 public:
  typedef MemoryFIFO::KeyType KeyType;
  typedef std::vector<std::pair<KeyType, NonEmptyString>> KeyValueContainer;

 protected:
  MemoryFIFOTest() : memory_fifo_(new MemoryFIFO(MemoryUsage(kDefaultMaxMemoryUsage))) {}

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

  NonEmptyString GenerateKeyValueData(KeyType& key, uint32_t size) {
    GenerateKeyValuePair generate_key_value_pair_(size);
    return boost::apply_visitor(generate_key_value_pair_, key);
  }

  std::unique_ptr<MemoryFIFO> memory_fifo_;
};

TEST_F(MemoryFIFOTest, BEH_Put) {
  KeyType key(GetRandomDataNameType()), temp_key;
  NonEmptyString value = GenerateKeyValueData(key, OneKB), temp_value, recovered;

  ASSERT_NO_THROW(memory_fifo_->Store(key, value));
  // Get first value.
  ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
  ASSERT_TRUE(recovered == value);

  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage - 1; ++i) {
    temp_key = GetRandomDataNameType();
    temp_value = GenerateKeyValueData(temp_key, OneKB);
    ASSERT_NO_THROW(memory_fifo_->Store(temp_key, temp_value));
    ASSERT_NO_THROW(recovered = memory_fifo_->Get(temp_key));
    ASSERT_TRUE(recovered == temp_value);
  }

  // Get first value again.
  ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
  ASSERT_TRUE(recovered == value);

  // Store another value to replace first.
  temp_key = GetRandomDataNameType();
  temp_value = GenerateKeyValueData(temp_key, OneKB);
  ASSERT_NO_THROW(memory_fifo_->Store(temp_key, temp_value));
  ASSERT_NO_THROW(recovered = memory_fifo_->Get(temp_key));
  ASSERT_TRUE(recovered == temp_value);

  // Try to get first value again.
  ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
  ASSERT_TRUE(recovered == value);
  // Should still equal last recovered value.
  ASSERT_TRUE(recovered != temp_value);
}

TEST_F(MemoryFIFOTest, BEH_Delete) {
  KeyValueContainer key_value_pairs;
  KeyType key;
  NonEmptyString value, recovered, temp(RandomAlphaNumericString(301));

  // Store some key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    key = GetRandomDataNameType();
    value = GenerateKeyValueData(key, (RandomUint32() % 300) + 1);
    key_value_pairs.push_back(std::make_pair(key, value));
    ASSERT_NO_THROW(memory_fifo_->Store(key, value));
    ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
    ASSERT_TRUE(recovered == value);
  }

  recovered = temp;

  // Delete stored key, value pairs and check they're gone.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    ASSERT_NO_THROW(memory_fifo_->Delete(key_value_pairs[i].first));
    EXPECT_THROW(recovered = memory_fifo_->Get(key_value_pairs[i].first), maidsafe_error);
    ASSERT_TRUE(recovered != key_value_pairs[i].second);
  }

  // Re-store same key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    ASSERT_NO_THROW(memory_fifo_->Store(key_value_pairs[i].first, key_value_pairs[i].second));
    ASSERT_NO_THROW(recovered = memory_fifo_->Get(key_value_pairs[i].first));
    ASSERT_TRUE(recovered == key_value_pairs[i].second);
  }

  recovered = temp;

  // Store some additional key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    key = GetRandomDataNameType();
    value = GenerateKeyValueData(key, (RandomUint32() % 300) + 1);
    key_value_pairs.push_back(std::make_pair(key, value));
    ASSERT_NO_THROW(memory_fifo_->Store(key, value));
    ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
    ASSERT_TRUE(recovered == value);
  }

  recovered = temp;

  // Check none of the original key, value pairs are present.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    EXPECT_THROW(recovered = memory_fifo_->Get(key_value_pairs[i].first), maidsafe_error);
    ASSERT_TRUE(recovered != key_value_pairs[i].second);
  }

  // Delete stored key, value pairs and check they're gone.
  for (uint32_t i = kDefaultMaxMemoryUsage; i != 2 * kDefaultMaxMemoryUsage; ++i) {
    ASSERT_NO_THROW(memory_fifo_->Delete(key_value_pairs[i].first));
    EXPECT_THROW(recovered = memory_fifo_->Get(key_value_pairs[i].first), maidsafe_error);
    ASSERT_TRUE(recovered != key_value_pairs[i].second);
  }
}

TEST_F(MemoryFIFOTest, BEH_FifoRepeatedlyStoreUsingSameKey) {
  const uint32_t size(50);
  KeyType key(GetRandomDataNameType());
  NonEmptyString value = GenerateKeyValueData(key, (RandomUint32() % size) + 1), recovered,
                 last_value;
  auto async =
      std::async(std::launch::async, [this, key, value] { memory_fifo_->Store(key, value); });
  ASSERT_NO_THROW(async.wait());
  ASSERT_TRUE(async.valid());
  ASSERT_NO_THROW(async.get());
  ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
  ASSERT_TRUE(value == recovered);

  uint32_t events((RandomUint32() % (2 * size)) + 10);
  for (uint32_t i = 0; i != events; ++i) {
    last_value = NonEmptyString(RandomAlphaNumericString((RandomUint32() % size) + 1));
    auto async = std::async(std::launch::async,
                            [this, key, last_value] { memory_fifo_->Store(key, last_value); });
    ASSERT_NO_THROW(async.wait());
    ASSERT_TRUE(async.valid());
    ASSERT_NO_THROW(async.get());
  }

  ASSERT_NO_THROW(recovered = memory_fifo_->Get(key));
  ASSERT_TRUE(value != recovered);
  ASSERT_TRUE(last_value == recovered);
}

TEST_F(MemoryFIFOTest, BEH_RandomAsync) {
  typedef KeyValueContainer::value_type value_type;

  KeyValueContainer key_value_pairs;
  uint32_t events((RandomUint32() % 400) + 100);
  std::vector<std::future<void>> future_stores, future_deletes;
  std::vector<std::future<NonEmptyString>> future_gets;

  for (uint32_t i = 0; i != events; ++i) {
    KeyType key(GetRandomDataNameType());
    NonEmptyString value = GenerateKeyValueData(key, (RandomUint32() % 300) + 1);
    key_value_pairs.push_back(std::make_pair(key, value));

    uint32_t event(RandomUint32() % 3);
    switch (event) {
      case 0: {
        if (!key_value_pairs.empty()) {
          KeyType event_key(key_value_pairs[RandomUint32() % key_value_pairs.size()].first);
          future_deletes.push_back(
              std::async([this, event_key] { memory_fifo_->Delete(event_key); }));
        } else {
          future_deletes.push_back(std::async([this, key] { memory_fifo_->Delete(key); }));
        }
        break;
      }
      case 1: {
        uint32_t index(i);
        KeyType event_key(key_value_pairs[index].first);
        NonEmptyString event_value(key_value_pairs[index].second);
        future_stores.push_back(std::async([this, event_key, event_value] {
          memory_fifo_->Store(event_key, event_value);
        }));
        break;
      }
      case 2: {
        if (!key_value_pairs.empty()) {
          KeyType event_key(key_value_pairs[RandomUint32() % key_value_pairs.size()].first);
          future_gets.push_back(
              std::async([this, event_key] { return memory_fifo_->Get(event_key); }));
        } else {
          future_gets.push_back(std::async([this, key] { return memory_fifo_->Get(key); }));
        }
        break;
      }
    }
  }

  for (auto& future_store : future_stores) {
    ASSERT_NO_THROW(future_store.get());
  }

  for (auto& future_delete : future_deletes) {
    try {
      future_delete.get();
    }
    catch (const std::exception& e) {
      std::string msg(e.what());
      LOG(kError) << msg;
    }
  }

  for (auto& future_get : future_gets) {
    try {
      NonEmptyString value(future_get.get());
      auto it = std::find_if(key_value_pairs.begin(), key_value_pairs.end(),
                             [this, &value](const value_type& key_value_pair) {
        return key_value_pair.second == value;
      });
      ASSERT_TRUE(key_value_pairs.end() != it);
    }
    catch (const std::exception& e) {
      std::string msg(e.what());
      LOG(kError) << msg;
    }
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
