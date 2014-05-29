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

#ifndef MAIDSAFE_COMMON_DATA_STORES_DATA_STORE_H_
#define MAIDSAFE_COMMON_DATA_STORES_DATA_STORE_H_

#include <deque>

#include "maidsafe/common/types.h"
#include "maidsafe/common/data_buffer.h"

namespace maidsafe {

namespace data_stores {

namespace fs = boost::filesystem;

template <typename StoragePolicy>
class DataStore : public StoragePolicy {
 public:
  typedef typename StoragePolicy::KeyType KeyType;
  typedef typename StoragePolicy::PopFunctor PopFunctor;

  explicit DataStore(MemoryUsage max_memory_usage) : StoragePolicy(max_memory_usage) {}

  DataStore(MemoryUsage max_memory_usage, PopFunctor pop_functor)
      : StoragePolicy(max_memory_usage, pop_functor) {}

  explicit DataStore(DiskUsage max_disk_usage) : StoragePolicy(max_disk_usage) {}

  DataStore(DiskUsage max_disk_usage, PopFunctor pop_functor)
      : StoragePolicy(max_disk_usage, pop_functor) {}

  DataStore(DiskUsage max_disk_usage, PopFunctor pop_functor, const fs::path& disk_path)
      : StoragePolicy(max_disk_usage, pop_functor, disk_path) {}

  DataStore(MemoryUsage max_memory_usage, DiskUsage max_disk_usage, PopFunctor pop_functor)
      : StoragePolicy(max_memory_usage, max_disk_usage, pop_functor) {}

  DataStore(MemoryUsage max_memory_usage, DiskUsage max_disk_usage, PopFunctor pop_functor,
            const fs::path& disk_path)
      : StoragePolicy(max_memory_usage, max_disk_usage, pop_functor, disk_path) {}

  ~DataStore() {}

  void Store(const KeyType& key, const NonEmptyString& value) { StoragePolicy::Store(key, value); }
  NonEmptyString Get(const KeyType& key) { return StoragePolicy::Get(key); }
  void Delete(const KeyType& key) { StoragePolicy::Delete(key); }

  template <typename T>
  void Store(const T& key, const NonEmptyString& value) {
    StoragePolicy::Store(key, value);
  }
  template <typename T>
  NonEmptyString Get(const T& key) {
    return StoragePolicy::Get(key);
  }
  template <typename T>
  void Delete(const T& key) {
    StoragePolicy::Delete(key);
  }

 private:
  DataStore(const DataStore&);
  DataStore& operator=(const DataStore&);
};

}  // namespace data_stores

}  // namespace maidsafe

#endif  // MAIDSAFE_COMMON_DATA_STORES_DATA_STORE_H_
