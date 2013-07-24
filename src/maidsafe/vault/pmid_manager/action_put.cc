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

#include "maidsafe/vault/pmid_manager/action_put.h"
#include "maidsafe/vault/pmid_manager/action_put.pb.h"


namespace maidsafe {
namespace vault {

const nfs::MessageAction ActionPmidManagerPut::kActionId(nfs::MessageAction::kPut);

ActionPmidManagerPut::ActionPmidManagerPut(const uint32_t size) : kSize(size) {}

ActionPmidManagerPut::ActionPmidManagerPut(const std::string& serialised_action)
  : kSize([&serialised_action]()->uint32_t {
        protobuf::ActionPmidManagerPut action_put_proto;
        action_put_proto.ParseFromString(serialised_action);
        return action_put_proto.size();
      }()) {}

ActionPmidManagerPut::ActionPmidManagerPut(const ActionPmidManagerPut& other)
  : kSize(other.kSize) {}

ActionPmidManagerPut::ActionPmidManagerPut(ActionPmidManagerPut&& other)
  : kSize(std::move(other.kSize)) {}

std::string ActionPmidManagerPut::Serialise() const {
  protobuf::ActionPmidManagerPut action_put_proto;
  action_put_proto.set_size(kSize);
  return action_put_proto.SerializeAsString();
}

void ActionPmidManagerPut::operator()(boost::optional<PmidManagerValue>& value) const {
  if (value)
    ThrowError(CommonErrors::invalid_parameter);
  value.reset(PmidManagerValue(kSize));
}

bool operator==(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
