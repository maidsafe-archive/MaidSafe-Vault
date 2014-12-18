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

#include "maidsafe/vault/parameters.h"

#include <exception>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {

namespace detail {

const int Parameters::kMinNetworkHealth(12);
size_t Parameters::max_recent_data_list_size(1000);
int Parameters::max_file_element_count(10000);
int Parameters::integrity_check_string_size(64);
const std::chrono::milliseconds Parameters::kDefaultTimeout(10000);
unsigned int Parameters::account_transfer_cleanup_factor(100);
std::chrono::seconds Parameters::account_transfer_life(60);
MemoryUsage Parameters::temp_store_size(100);
unsigned int Parameters::max_replication_factor(routing::Parameters::closest_nodes_size / 2);
unsigned int Parameters::min_replication_factor(routing::Parameters::group_size);

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
