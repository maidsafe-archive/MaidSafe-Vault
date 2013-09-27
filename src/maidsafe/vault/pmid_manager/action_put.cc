/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/pmid_manager/action_put.h"
#include "maidsafe/vault/pmid_manager/action_put.pb.h"

namespace maidsafe {
namespace vault {

const nfs::MessageAction ActionPmidManagerPut::kActionId(nfs::MessageAction::kPutRequest);

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
  if (!value)
    value.reset(PmidManagerValue());
}

bool operator==(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
