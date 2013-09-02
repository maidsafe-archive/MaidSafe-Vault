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

#include "maidsafe/vault/data_manager/action_decrement_subscribers.h"
#include "maidsafe/vault/data_manager/action_decrement_subscribers.pb.h"

#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {
namespace vault {

const nfs::MessageAction ActionDataManagerDecrementSubscribers::kActionId;

std::string ActionDataManagerDecrementSubscribers::Serialise() const {
  protobuf::ActionDataManagerDecrementSubscribers action_decrement_subscribers_proto;
  return action_decrement_subscribers_proto.SerializeAsString();
}

void ActionDataManagerDecrementSubscribers::operator()(
    boost::optional<DataManagerValue>& value) {
  if (value)
    value->DecrementSubscribers();
  else
    ThrowError(CommonErrors::invalid_parameter);
}

bool operator==(const ActionDataManagerDecrementSubscribers& /*lhs*/,
                const ActionDataManagerDecrementSubscribers& /*rhs*/) {
  return true;
}

bool operator!=(const ActionDataManagerDecrementSubscribers& lhs,
                const ActionDataManagerDecrementSubscribers& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
