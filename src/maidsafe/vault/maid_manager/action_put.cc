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

#include "maidsafe/vault/maid_manager/action_put.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/maid_manager/action_put.pb.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/maid_manager/value.h"

namespace maidsafe {

namespace vault {

ActionMaidManagerPut::ActionMaidManagerPut(int32_t cost) : kCost(cost) {}

ActionMaidManagerPut::ActionMaidManagerPut(const std::string& serialised_action)
    : kCost([&serialised_action]()->int32_t {
        protobuf::ActionMaidManagerPut action_put_proto;
        if (!action_put_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
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

detail::DbAction ActionMaidManagerPut::operator()(MaidManagerMetadata& metadata,
                                                  std::unique_ptr<MaidManagerValue>& value) const {
  if (!value)
    value.reset(new MaidManagerValue());
  value->Put(kCost);
  metadata.PutData(kCost);
  return detail::DbAction::kPut;
}

bool operator==(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs) {
  return lhs.kCost == rhs.kCost;
}

bool operator!=(const ActionMaidManagerPut& lhs, const ActionMaidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
