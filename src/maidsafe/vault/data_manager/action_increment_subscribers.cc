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

#include "maidsafe/vault/data_manager/action_increment_subscribers.h"
#include "maidsafe/vault/pmid_manager/action_increment_subscribers.pb.h"


namespace maidsafe {
namespace vault {

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const PmidNode& pmid_node, const uint32_t& size)
    : kPmidNode(pmid_node),
      kSize(size) {}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const std::string& serialised_action)
    : kSize(0) {
  protobuf::ActionDataManagerIncrementSubscribers action_increment_subscribers_proto;
  action_increment_subscribers_proto.ParseFromString(serialised_action);
  kPmidName = action_increment_subscribers_proto.pmid_name();
  kSize = action_increment_subscribers_proto.size();
}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const ActionDataManagerIncrementSubscribers& other)
    : kPmidName(other.kPmidName),
      kSize(other.kSize) {}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    ActionDataManagerIncrementSubscribers&& other)
    : kPmidName(std::move(other.kPmidName)),
      kSize(std::move(other.kSize)) {}

std::string ActionDataManagerIncrementSubscribers::Serialise() const {
  protobuf::ActionDataManagerIncrementSubscribers action_increment_subscribers_proto;
  action_increment_subscribers_proto.set_size(kPmidName);
  action_increment_subscribers_proto.set_size(kSize);
  return action_increment_subscribers_proto.SerializeAsString();
}

void ActionDataManagerIncrementSubscribers::operator()(
    boost::optional<PmidManagerValue>& value) const {
  if (value)
    value.IncrementSubscribers();
  else
    value.reset(PmidManagerValue(kPmidName, kSize));
}

bool operator==(const ActionDataManagerIncrementSubscribers& lhs,
                const ActionDataManagerIncrementSubscribers& rhs) {
  return lhs.kPmidName == rhs.kPmidName && lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionDataManagerIncrementSubscribers& lhs,
                const ActionDataManagerIncrementSubscribers& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
