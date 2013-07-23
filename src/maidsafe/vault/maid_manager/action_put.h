/* Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_PUT_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_PUT_H_

#include <cstdint>
#include <string>

#include "boost/optional/optional.hpp"

#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class MaidManagerValue;

struct ActionMaidManagerPut {
  explicit ActionMaidManagerPut(int32_t cost);
  explicit ActionMaidManagerPut(const std::string& serialised_action);
  ActionMaidManagerPut(const ActionMaidManagerPut& other);
  ActionMaidManagerPut(ActionMaidManagerPut&& other);
  std::string Serialise() const;

  void operator()(boost::optional<MaidManagerValue>& value) const;

  static const nfs::MessageAction kActionId;
  const int32_t kCost;

 private:
  ActionMaidManagerPut();
  ActionMaidManagerPut& operator=(ActionMaidManagerPut other);
};

bool operator==(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs);

bool operator!=(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_PUT_H_
