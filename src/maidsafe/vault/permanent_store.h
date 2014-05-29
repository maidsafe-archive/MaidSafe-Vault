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

#ifndef MAIDSAFE_COMMON_DATA_STORES_PERMANENT_STORE_H_
#define MAIDSAFE_COMMON_DATA_STORES_PERMANENT_STORE_H_

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <utility>
#include <deque>
#include <set>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/variant.hpp"

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_name_variant.h"

namespace maidsafe {

namespace data_stores {

namespace test {
class PermanentStoreTest;
}

class PermanentStore {
 public:
  typedef DataNameVariant KeyType;

  PermanentStore(const boost::filesystem::path& disk_path, DiskUsage max_disk_usage);
  ~PermanentStore();

  void Put(const KeyType& key, const NonEmptyString& value);
  void Delete(const KeyType& key);
  NonEmptyString Get(const KeyType& key) const;

  // Return list of elements that should have but not exists yet
  std::vector<KeyType> ElementsToStore(std::set<KeyType> element_list);

  void SetMaxDiskUsage(DiskUsage max_disk_usage);

  DiskUsage GetMaxDiskUsage() const { return max_disk_usage_; }
  DiskUsage GetCurrentDiskUsage() const { return current_disk_usage_; }
  boost::filesystem::path GetDiskPath() const { return kDiskPath_; }
  std::vector<KeyType> GetKeys() const;

  friend class test::PermanentStoreTest;

 private:
  PermanentStore(const PermanentStore&);
  PermanentStore& operator=(const PermanentStore&);

  boost::filesystem::path GetFilePath(const KeyType& key) const;
  bool HasDiskSpace(uint64_t required_space) const;
  boost::filesystem::path KeyToFilePath(const KeyType& key) const;

  const boost::filesystem::path kDiskPath_;
  DiskUsage max_disk_usage_, current_disk_usage_;
  const uint32_t kDepth_;
  mutable std::mutex mutex_;
  GetIdentityVisitor get_identity_visitor_;
};

}  // namespace data_stores

}  // namespace maidsafe

#endif  // MAIDSAFE_COMMON_DATA_STORES_PERMANENT_STORE_H_
