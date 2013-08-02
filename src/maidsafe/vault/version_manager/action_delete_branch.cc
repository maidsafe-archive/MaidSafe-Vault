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

#include "maidsafe/vault/version_manager/action_delete_branch.h"
#include "maidsafe/vault/version_manager/action_delete_branch.pb.h"


namespace maidsafe {

namespace vault {

ActionDeleteBranch::ActionDeleteBranch(const std::string& serialised_action)
    : version_name_([&serialised_action]() {
                      protobuf::ActionDeleteBranch action_delete_branch_proto;
                      if (!action_delete_branch_proto.ParseFromString(serialised_action))
                        ThrowError(CommonErrors::parsing_error);
                      return StructuredDataVersions::VersionName(
                          action_delete_branch_proto.serialised_version_name());
                    }()) {}

ActionDeleteBranch::ActionDeleteBranch(
    const StructuredDataVersions::VersionName& version_name)
    : version_name_(version_name) {}

ActionDeleteBranch::ActionDeleteBranch(const ActionDeleteBranch& other)
    : version_name(other.version_name) {}

ActionDeleteBranch::ActionDeleteBranch(ActionDeleteBranch&& other)
    : version_name(std::move(other.version_name)) {}

std::string ActionDeleteBranch::Serialise() const {
  protobuf::ActionDeleteBranch action_delete_branch_proto;
  action_delete_branch_proto.set_serialised_version_name(version_name.Serialise());
  return action_delete_branch_proto.SerializeAsString();
}

void ActionDeleteBranch::operator()(boost::optional<VersionManagerValue> value) const {
  value->DeleteBranch();
}

bool operator==(const ActionDeleteBranch& lhs, const ActionDeleteBranch& rhs) {
  return lhs.version_name == rhs.version_name;
}

bool operator!=(const ActionDeleteBranch& lhs, const ActionDeleteBranch& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
