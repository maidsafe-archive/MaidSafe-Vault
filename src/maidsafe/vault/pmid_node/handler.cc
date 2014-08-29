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

#include "maidsafe/vault/pmid_node/handler.h"

namespace maidsafe {
namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage / 5);
DiskUsage perm_disk_usage = DiskUsage(10000000000);


// MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
//  fs::space_info space = fs::space("/tmp/vault_root_dir\\");  // FIXME  NOLINT

//  DiskUsage disk_total = DiskUsage(space.available);
//  DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//  DiskUsage cache_size = DiskUsage(disk_total * 0.1);

}  // namespace

PmidNodeHandler::PmidNodeHandler(const boost::filesystem::path vault_root_dir,
                                 DiskUsage max_disk_usage)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      chunk_store_(vault_root_dir / "pmid_node" / "permanent", max_disk_usage) {}
// TODO(Fraser) BEFORE_RELEASE need to decide on propertion of max_disk_usage. As leveldb and cache
// will be using a share of it
boost::filesystem::path PmidNodeHandler::GetDiskPath() const {
  return chunk_store_.GetDiskPath();
}

std::vector<DataNameVariant> PmidNodeHandler::GetAllDataNames() const {
  return chunk_store_.GetKeys();
}

DiskUsage PmidNodeHandler::AvailableSpace() const {
  return disk_total_;
}

}  // namespace vault
}  // namespace maidsafe
