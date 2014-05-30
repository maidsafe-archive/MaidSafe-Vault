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

#include "maidsafe/common/data_stores/memory_buffer.h"

#include <memory>

#include "maidsafe/common/error.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/data_stores/tests/test_utils.h"

namespace maidsafe {

namespace data_stores {

namespace test {

const uint64_t kDefaultMaxMemoryUsage(10);  // elements
const uint64_t OneKB(1024);

class MemoryBufferTest {
 public:
  typedef MemoryBuffer::KeyType KeyType;
  typedef std::vector<std::pair<KeyType, NonEmptyString>> KeyValueContainer;

 protected:
  MemoryBufferTest() : memory_buffer_(new MemoryBuffer(MemoryUsage(kDefaultMaxMemoryUsage))) {}

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

  std::unique_ptr<MemoryBuffer> memory_buffer_;
};

TEST_CASE_METHOD(MemoryBufferTest, "Store", "[Behavioural]") {
  KeyType key(GetRandomDataNameType()), temp_key;
  NonEmptyString value = GenerateKeyValueData(key, OneKB), temp_value, recovered;

  REQUIRE_NOTHROW(memory_buffer_->Store(key, value));
  // Get first value.
  REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
  REQUIRE(recovered == value);

  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage - 1; ++i) {
    temp_key = GetRandomDataNameType();
    temp_value = GenerateKeyValueData(temp_key, OneKB);
    REQUIRE_NOTHROW(memory_buffer_->Store(temp_key, temp_value));
    REQUIRE_NOTHROW(recovered = memory_buffer_->Get(temp_key));
    REQUIRE(recovered == temp_value);
  }

  // Get first value again.
  REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
  REQUIRE(recovered == value);

  // Store another value to replace first.
  temp_key = GetRandomDataNameType();
  temp_value = GenerateKeyValueData(temp_key, OneKB);
  REQUIRE_NOTHROW(memory_buffer_->Store(temp_key, temp_value));
  REQUIRE_NOTHROW(recovered = memory_buffer_->Get(temp_key));
  REQUIRE(recovered == temp_value);

  // Try to get first value again.
  REQUIRE_THROWS_AS(recovered = memory_buffer_->Get(key), maidsafe_error);
  REQUIRE(recovered != value);
  // Should still equal last recovered value.
  REQUIRE(recovered == temp_value);
}

TEST_CASE_METHOD(MemoryBufferTest, "Delete", "[Behavioural]") {
  KeyValueContainer key_value_pairs;
  KeyType key;
  NonEmptyString value, recovered, temp(RandomAlphaNumericString(301));

  // Store some key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    key = GetRandomDataNameType();
    value = GenerateKeyValueData(key, (RandomUint32() % 300) + 1);
    key_value_pairs.push_back(std::make_pair(key, value));
    REQUIRE_NOTHROW(memory_buffer_->Store(key, value));
    REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
    REQUIRE(recovered == value);
  }

  recovered = temp;

  // Delete stored key, value pairs and check they're gone.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    REQUIRE_NOTHROW(memory_buffer_->Delete(key_value_pairs[i].first));
    REQUIRE_THROWS_AS(recovered = memory_buffer_->Get(key_value_pairs[i].first), maidsafe_error);
    REQUIRE(recovered != key_value_pairs[i].second);
  }

  // Re-store same key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    REQUIRE_NOTHROW(memory_buffer_->Store(key_value_pairs[i].first, key_value_pairs[i].second));
    REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key_value_pairs[i].first));
    REQUIRE(recovered == key_value_pairs[i].second);
  }

  recovered = temp;

  // Store some additional key, value pairs.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    key = GetRandomDataNameType();
    value = GenerateKeyValueData(key, (RandomUint32() % 300) + 1);
    key_value_pairs.push_back(std::make_pair(key, value));
    REQUIRE_NOTHROW(memory_buffer_->Store(key, value));
    REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
    REQUIRE(recovered == value);
  }

  recovered = temp;

  // Check none of the original key, value pairs are present.
  for (uint32_t i = 0; i != kDefaultMaxMemoryUsage; ++i) {
    REQUIRE_THROWS_AS(recovered = memory_buffer_->Get(key_value_pairs[i].first), maidsafe_error);
    REQUIRE(recovered != key_value_pairs[i].second);
  }

  // Delete stored key, value pairs and check they're gone.
  for (uint32_t i = kDefaultMaxMemoryUsage; i != 2 * kDefaultMaxMemoryUsage; ++i) {
    REQUIRE_NOTHROW(memory_buffer_->Delete(key_value_pairs[i].first));
    REQUIRE_THROWS_AS(recovered = memory_buffer_->Get(key_value_pairs[i].first), maidsafe_error);
    REQUIRE(recovered != key_value_pairs[i].second);
  }
}

TEST_CASE_METHOD(MemoryBufferTest, "RepeatedlyStoreUsingSameKey", "[Behavioural]") {
  const uint32_t size(50);
  KeyType key(GetRandomDataNameType());
  NonEmptyString value = GenerateKeyValueData(key, (RandomUint32() % size) + 1), recovered,
                 last_value;
  auto async =
      std::async(std::launch::async, [this, key, value] { memory_buffer_->Store(key, value); });
  REQUIRE_NOTHROW(async.wait());
  REQUIRE(async.valid());
  REQUIRE_NOTHROW(async.get());
  REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
  REQUIRE(value == recovered);

  uint32_t events(RandomUint32() % (2 * size));
  for (uint32_t i = 0; i != events; ++i) {
    last_value = NonEmptyString(RandomAlphaNumericString((RandomUint32() % size) + 1));
    auto async = std::async(std::launch::async,
                            [this, key, last_value] { memory_buffer_->Store(key, last_value); });
    REQUIRE_NOTHROW(async.wait());
    REQUIRE(async.valid());
    REQUIRE_NOTHROW(async.get());
  }

  REQUIRE_NOTHROW(recovered = memory_buffer_->Get(key));
  REQUIRE(value != recovered);
  REQUIRE(last_value == recovered);
}

TEST_CASE_METHOD(MemoryBufferTest, "RandomAsync", "[Behavioural]") {
  typedef KeyValueContainer::value_type value_type;

  KeyValueContainer key_value_pairs;
  uint32_t events(RandomUint32() % 500);
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
              std::async([this, event_key] { memory_buffer_->Delete(event_key); }));
        } else {
          future_deletes.push_back(std::async([this, key] { memory_buffer_->Delete(key); }));
        }
        break;
      }
      case 1: {
        uint32_t index(i);
        KeyType event_key(key_value_pairs[index].first);
        NonEmptyString event_value(key_value_pairs[index].second);
        future_stores.push_back(std::async([this, event_key, event_value] {
          memory_buffer_->Store(event_key, event_value);
        }));
        break;
      }
      case 2: {
        if (!key_value_pairs.empty()) {
          KeyType event_key(key_value_pairs[RandomUint32() % key_value_pairs.size()].first);
          future_gets.push_back(
              std::async([this, event_key] { return memory_buffer_->Get(event_key); }));
        } else {
          future_gets.push_back(std::async([this, key] { return memory_buffer_->Get(key); }));
        }
        break;
      }
    }
  }

  for (auto& future_store : future_stores) {
    REQUIRE_NOTHROW(future_store.get());
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
                             [this, &value](const value_type & key_value_pair) {
        return key_value_pair.second == value;
      });
      REQUIRE(key_value_pairs.end() != it);
    }
    catch (const std::exception& e) {
      std::string msg(e.what());
      LOG(kError) << msg;
    }
  }
}

}  // namespace test

}  // namespace data_stores

}  // namespace maidsafe
