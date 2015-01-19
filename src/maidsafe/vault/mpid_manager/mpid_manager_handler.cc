/*  Copyright 2015 MaidSafe.net limited

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

#include "maidsafe/vault/mpid_manager/mpid_manager_handler.h"

namespace maidsafe {

namespace vault {

MpidManagerHandler::MpidManagerHandler(const boost::filesystem::path vault_root_dir,
                                       DiskUsage max_disk_usage)
    : chunk_store_(vault_root_dir / "mpid_manager" / "permanent", max_disk_usage) {}
// TODO(Fraser) BEFORE_RELEASE need to decide on propertion of max_disk_usage. As sqlite and cache
// will be using a share of it

}  // namespace vault

}  // namespace maidsafe
