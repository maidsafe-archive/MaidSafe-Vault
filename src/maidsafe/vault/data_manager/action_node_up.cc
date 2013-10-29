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

#include "maidsafe/vault/data_manager/action_node_up.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/data_manager/action_node_up.pb.h"
#include "maidsafe/vault/data_manager/value.h"

namespace maidsafe {

namespace vault {

ActionDataManagerNodeUp::ActionDataManagerNodeUp(const PmidName& pmid_name)
    : kPmidName(pmid_name) {}

ActionDataManagerNodeUp::ActionDataManagerNodeUp(const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerNodeUp action_set_pmid_online_proto;
        if (!action_set_pmid_online_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return PmidName(Identity(action_set_pmid_online_proto.pmid_name()));
      }()) {}

ActionDataManagerNodeUp::ActionDataManagerNodeUp(const ActionDataManagerNodeUp& other)
    : kPmidName(other.kPmidName) {}

ActionDataManagerNodeUp::ActionDataManagerNodeUp(ActionDataManagerNodeUp&& other)
    : kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerNodeUp::Serialise() const {
  protobuf::ActionDataManagerNodeUp action_set_pmid_online_proto;
  action_set_pmid_online_proto.set_pmid_name(kPmidName->string());
  return action_set_pmid_online_proto.SerializeAsString();
}

detail::DbAction ActionDataManagerNodeUp::operator()(std::unique_ptr<DataManagerValue>& value) {
  if (value) {
    value->SetPmidOnline(kPmidName);
    return detail::DbAction::kPut;
  }

  ThrowError(CommonErrors::no_such_element);
  return detail::DbAction::kDelete;
}

bool operator==(const ActionDataManagerNodeUp& lhs, const ActionDataManagerNodeUp& rhs) {
  return lhs.kPmidName == rhs.kPmidName;
}

bool operator!=(const ActionDataManagerNodeUp& lhs, const ActionDataManagerNodeUp& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
