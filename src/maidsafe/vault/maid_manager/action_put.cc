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

#include "maidsafe/vault/maid_manager/action_put.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/maid_manager/action_put.pb.h"
#include "maidsafe/vault/maid_manager/value.h"


namespace maidsafe {

namespace vault {

const nfs::MessageAction ActionMaidManagerPut::kActionId(nfs::MessageAction::kPut);

ActionMaidManagerPut::ActionMaidManagerPut(int32_t cost) : kCost(cost) {}

ActionMaidManagerPut::ActionMaidManagerPut(const std::string& serialised_action)
    : kCost([&serialised_action]()->int32_t {
        protobuf::ActionMaidManagerPut action_put_proto;
        if (!action_put_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return action_put_proto.cost();
      }()) {}

ActionMaidManagerPut::ActionMaidManagerPut(const ActionMaidManagerPut& other)
    : kCost(other.kCost) {}

ActionMaidManagerPut::ActionMaidManagerPut(ActionMaidManagerPut&& other)
    : kCost(std::move(other.kCost)) {}

std::string ActionMaidManagerPut::Serialise() const {
  protobuf::ActionMaidManagerPut action_put_proto;
  action_put_proto.set_cost(kCost);
  return action_put_proto.SerializeAsString();
}

void ActionMaidManagerPut::operator()(boost::optional<MaidManagerValue>& value) const {
  if (!value)
    value.reset(MaidManagerValue());
  value->Put(kCost);
}

bool operator==(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs) {
  return lhs.kCost == rhs.kCost;
}

bool operator!=(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
