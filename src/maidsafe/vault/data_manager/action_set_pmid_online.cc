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

#include "maidsafe/vault/data_manager/action_set_pmid_online.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/data_manager/action_set_pmid_online.h"
#include "maidsafe/vault/data_manager/metadata.h"
#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {

namespace vault {

ActionDataManagerSetPmidOnline::ActionDataManagerSetPmidOnline(const PmidName& pmid_name)
    : kPmidName(pmid_name) {}

ActionDataManagerSetPmidOnline::ActionDataManagerSetPmidOnline(const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerSetPmidOnline action_set_pmid_online_proto;
        if (!action_set_pmid_online_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return PmidName(Identity(action_set_pmid_online_proto.pmid_name()));
      }()) {}

ActionDataManagerSetPmidOnline::ActionDataManagerSetPmidOnline(const ActionDataManagerSetPmidOnline& other)
    : kPmidName(other.kPmidName) {}

ActionDataManagerSetPmidOnline::ActionDataManagerSetPmidOnline(
    ActionDataManagerSetPmidOnline&& other)
    : kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerSetPmidOnline::Serialise() const {
  protobuf::ActionDataManagerSetPmidOnline action_set_pmid_online_proto;
  action_set_pmid_online_proto.set_pmid_name(kPmidName);
  return action_set_pmid_online_proto.SerializeAsString();
}

void ActionDataManagerSetPmidOnline::operator()(boos::optional<DataManagerValue>& value) const {
  if (!value)
    ThrowError(CommonErrors::invalid_parameter);
  value->SetPmidOnline(kPmidName);
}

bool operator==(const ActionDataManagerSetPmidOnline& lhs,
    const ActionDataManagerSetPmidOnline& rhs) {
  return lhs.kPmidName == rhs.kPmidName;
}

bool operator!=(const ActionDataManagerSetPmidOnline& lhs,
    const ActionDataManagerSetPmidOnline& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
