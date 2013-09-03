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

#include "maidsafe/vault/data_manager/action_add_pmid.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/data_manager/action_add_pmid.pb.h"
#include "maidsafe/vault/data_manager/metadata.h"
#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {

namespace vault {

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const PmidName& pmid_name)
    : kPmidName(pmid_name) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
        if (!action_add_pmid_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return PmidName(Identity(action_add_pmid_proto.pmid_name()));
      }()) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const ActionDataManagerAddPmid& other)
    : kPmidName(other.kPmidName) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(ActionDataManagerAddPmid&& other)
    : kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerAddPmid::Serialise() const {
  protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
  action_add_pmid_proto.set_pmid_name(kPmidName->string());
  return action_add_pmid_proto.SerializeAsString();
}

void ActionDataManagerAddPmid::operator()(boost::optional<DataManagerValue>& value) {
  if (!value)
    value.reset(new DataManagerValue(kPmidName, ));
  value->AddPmid(kPmidName);
}

bool operator==(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return lhs.kPmidName == rhs.kPmidName;
}

bool operator!=(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
