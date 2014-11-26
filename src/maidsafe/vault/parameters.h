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

#ifndef MAIDSAFE_VAULT_PARAMETERS_H_
#define MAIDSAFE_VAULT_PARAMETERS_H_

#include <cstddef>
#include <chrono>

#include "maidsafe/common/types.h"

namespace maidsafe {

namespace vault {

namespace detail {

struct Parameters {
 public:
  // Min % returned by routing.network_status() to consider this node still online.
  static const int kMinNetworkHealth;
  // Max number of recent actions in account classes.
  static size_t max_recent_data_list_size;
  // Max count of elements allowed in each account file
  static int max_file_element_count;
  // Size of the string passed for integrity checking
  static int integrity_check_string_size;
  // Default network timeout
  static const std::chrono::milliseconds kDefaultTimeout;
  // Prune account transfer when the size is divisible by below factor
  static unsigned int account_transfer_cleanup_factor;
  // Removes entries which have been longer than below factor
  static std::chrono::seconds account_transfer_life;
  // Maximum number of chunks stored in data manager temporary store
  static MemoryUsage temp_store_size;
  // Maximum number of pmids storing a chunk
  static unsigned int max_replication_factor;
  // Minimum required number of online pmids for a chunk
  static unsigned int min_replication_factor;

 private:
  Parameters();
  ~Parameters();
  Parameters(const Parameters&);
  Parameters& operator=(const Parameters&);
  Parameters(const Parameters&&);
  Parameters& operator=(Parameters&&);
};

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PARAMETERS_H_
