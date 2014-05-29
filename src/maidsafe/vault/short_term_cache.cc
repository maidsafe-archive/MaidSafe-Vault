/*  Copyright 2013 MaidSafe.net limited

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

namespace maidsafe {

namespace data_stores {

MemoryBuffer::MemoryBuffer(MemoryUsage max_memory_usage)
    : memory_buffer_(static_cast<uint32_t>(max_memory_usage.data)), mutex_() {}

MemoryBuffer::~MemoryBuffer() {}

void MemoryBuffer::Store(const KeyType& key, const NonEmptyString& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr != memory_buffer_.end())
    memory_buffer_.erase(itr);
  memory_buffer_.push_back(std::make_pair(key, value));
}

NonEmptyString MemoryBuffer::Get(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr == std::end(memory_buffer_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  return itr->second;
}

void MemoryBuffer::Delete(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr == std::end(memory_buffer_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  memory_buffer_.erase(itr);
}

MemoryBuffer::MemoryBufferType::iterator MemoryBuffer::Find(const KeyType& key) {
  return std::find_if(
      std::begin(memory_buffer_), std::end(memory_buffer_),
      [&key](const MemoryBufferType::value_type & key_value) { return key_value.first == key; });
}

}  // namespace data_stores

}  // namespace maidsafe
