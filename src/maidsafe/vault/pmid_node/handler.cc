/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

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
DiskUsage perm_disk_usage = DiskUsage(10000);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);

MemoryUsage mem_only_cache_usage = MemoryUsage(100);  // size in elements

// MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
//  fs::space_info space = fs::space("/tmp/vault_root_dir\\");  // FIXME  NOLINT

//  DiskUsage disk_total = DiskUsage(space.available);
//  DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//  DiskUsage cache_size = DiskUsage(disk_total * 0.1);

}  // namespace

PmidNodeHandler::PmidNodeHandler(const boost::filesystem::path vault_root_dir)
  : space_info_(boost::filesystem::space(vault_root_dir)),
    disk_total_(space_info_.available),
    permanent_size_(disk_total_ * 4 / 5),
    cache_size_(disk_total_ / 10),
    permanent_data_store_(vault_root_dir / "pmid_node" / "permanent", DiskUsage(10000)),  // TODO(Fraser) BEFORE_RELEASE need to read value from disk
    cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                      vault_root_dir / "pmid_node" / "cache"),  // FIXME - DiskUsage  NOLINT
    mem_only_cache_(mem_only_cache_usage) {
}

boost::filesystem::path PmidNodeHandler::GetPermanentStorePath() const {
  return permanent_data_store_.GetDiskPath();
}

}  // namespace vault
}  // namespace maidsafe
