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

#ifndef MAIDSAFE_COMMON_DATA_STORES_MEMORY_BUFFER_H_
#define MAIDSAFE_COMMON_DATA_STORES_MEMORY_BUFFER_H_

#include <mutex>
#include <utility>

#include "boost/circular_buffer.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_name_variant.h"

namespace maidsafe {

namespace data_stores {

namespace test {

class MemoryBufferTest;

}  // namespace test

class MemoryBuffer {
 public:
  typedef DataNameVariant KeyType;
  typedef boost::circular_buffer<std::pair<KeyType, NonEmptyString>> MemoryBufferType;

  explicit MemoryBuffer(MemoryUsage max_memory_usage);

  ~MemoryBuffer();

  void Store(const KeyType& key, const NonEmptyString& value);
  NonEmptyString Get(const KeyType& key);
  void Delete(const KeyType& key);

 private:
  MemoryBuffer(const MemoryBuffer&);
  MemoryBuffer& operator=(const MemoryBuffer&);

  MemoryBufferType::iterator Find(const KeyType& key);

  MemoryBufferType memory_buffer_;
  mutable std::mutex mutex_;
};

}  // namespace data_stores

}  // namespace maidsafe

#endif  // MAIDSAFE_COMMON_DATA_STORES_MEMORY_BUFFER_H_
