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

#include "maidsafe/vault/memory_fifo.h"

namespace maidsafe {

namespace vault {

MemoryFIFO::MemoryFIFO(MemoryUsage max_memory_usage)
    : memory_fifo_(static_cast<uint32_t>(max_memory_usage.data)), mutex_() {}

void MemoryFIFO::Store(const KeyType& key, const NonEmptyString& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr != memory_fifo_.end())
    memory_fifo_.erase(itr);
  memory_fifo_.push_back(std::make_pair(key, value));
}

NonEmptyString MemoryFIFO::Get(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr == std::end(memory_fifo_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  auto result = *itr;
  memory_fifo_.erase(itr);
  memory_fifo_.push_back(result);  // last out
  return result.second;
}

void MemoryFIFO::Delete(const KeyType& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr(Find(key));
  if (itr == std::end(memory_fifo_))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  memory_fifo_.erase(itr);
}

MemoryFIFO::MemoryFIFOType::iterator MemoryFIFO::Find(const KeyType& key) {
  return std::find_if(
      std::begin(memory_fifo_), std::end(memory_fifo_),
      [&key](const MemoryFIFOType::value_type & key_value) { return key_value.first == key; });
}

}  // namespace vault

}  // namespace maidsafe
