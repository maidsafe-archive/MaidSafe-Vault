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

#ifndef MAIDSAFE_VAULT_CHUNK_STORE_H_
#define MAIDSAFE_VAULT_CHUNK_STORE_H_

#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data.h"
#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/data_types/mutable_data.h"
#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault {

class ChunkStore {
 public:
  using NameType = Data::NameAndTypeId;

  ChunkStore(const boost::filesystem::path& disk_path, DiskUsage max_disk_usage);
  ~ChunkStore();
  ChunkStore(const ChunkStore&) = delete;
  ChunkStore(ChunkStore&&) = delete;
  ChunkStore& operator=(const ChunkStore&) = delete;
  ChunkStore& operator=(ChunkStore&&) = delete;

  void Put(const NameType& name, const NonEmptyString& value);
  void Delete(const NameType& name);
  NonEmptyString Get(const NameType& name) const;

  void SetMaxDiskUsage(DiskUsage max_disk_usage);

  DiskUsage MaxDiskUsage() const { return max_disk_usage_; }
  DiskUsage CurrentDiskUsage() const { return current_disk_usage_; }
  boost::filesystem::path DiskPath() const { return kDiskPath_; }
  std::vector<NameType> Names() const;

 private:
  bool HasDiskSpace(std::uint64_t required_space) const;
  boost::filesystem::path NameToFilePath(NameType name) const;
  void GetNames(const boost::filesystem::path& path, std::string prefix,
                std::vector<NameType>& names) const;
  NameType ComposeName(std::string file_name_str) const;

  const boost::filesystem::path kDiskPath_;
  DiskUsage max_disk_usage_, current_disk_usage_;
  const std::uint32_t kDepth_;
  mutable std::mutex mutex_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CHUNK_STORE_H_
