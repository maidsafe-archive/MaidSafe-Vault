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

#include "maidsafe/vault/data_manager/action_node_down.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/data_manager/action_node_down.pb.h"
#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {

namespace vault {

ActionDataManagerNodeDown::ActionDataManagerNodeDown(const PmidName& pmid_name)
    : kPmidName(pmid_name) {}

ActionDataManagerNodeDown::ActionDataManagerNodeDown(
    const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerNodeDown action_node_down_proto;
        if (!action_node_down_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return PmidName(Identity(action_node_down_proto.pmid_name()));
      }()) {}

ActionDataManagerNodeDown::ActionDataManagerNodeDown(
    const ActionDataManagerNodeDown& other)
    : kPmidName(other.kPmidName) {}

ActionDataManagerNodeDown::ActionDataManagerNodeDown(
    ActionDataManagerNodeDown&& other)
    : kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerNodeDown::Serialise() const {
  protobuf::ActionDataManagerNodeDown action_node_down_proto;
  action_node_down_proto.set_pmid_name(kPmidName->string());
  return action_node_down_proto.SerializeAsString();
}

void ActionDataManagerNodeDown::operator()(boost::optional<DataManagerValue>& value) const {
  if (!value)
    ThrowError(CommonErrors::invalid_parameter);
  value->SetPmidOffline(kPmidName);
}

bool operator==(const ActionDataManagerNodeDown& lhs, const ActionDataManagerNodeDown& rhs) {
  return lhs.kPmidName == rhs.kPmidName;
}

bool operator!=(const ActionDataManagerNodeDown& lhs, const ActionDataManagerNodeDown& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
