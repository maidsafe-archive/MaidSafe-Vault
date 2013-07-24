/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_PARAMETERS_H_
#define MAIDSAFE_VAULT_PARAMETERS_H_

#include <cstddef>


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
