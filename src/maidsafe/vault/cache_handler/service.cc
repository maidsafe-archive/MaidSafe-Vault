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

#include "maidsafe/vault/cache_handler/service.h"


namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(100);  // size in elements
DiskUsage cache_size = DiskUsage(200);

}

CacheHandlerService::CacheHandlerService(routing::Routing& routing,
                                         const boost::filesystem::path vault_root_dir)
    : routing_(routing),
      cache_size_(cache_size),
      cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                        vault_root_dir / "cache" / "cache"),
      mem_only_cache_(mem_only_cache_usage) {}

}  // namespace vault

}  // namespace maidsafe
