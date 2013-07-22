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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_

#include <cstdint>
#include <string>


namespace maidsafe {

namespace vault {

class MaidManagerValue {
 public:
  explicit MaidManagerValue(const std::string& serialised_maid_manager_value);
  MaidManagerValue();
  std::string Serialise() const;

  void Put(int32_t cost);
  void Delete(int32_t cost);
  int32_t count() const { return count_; }
  int32_t cost() const { return cost_; }

 private:
  int32_t count_, cost_;
};

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
