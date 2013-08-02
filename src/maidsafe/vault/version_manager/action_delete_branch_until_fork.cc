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

#include "maidsafe/vault/version_manager/action_delete_branch_until_fork.h"
#include "maidsafe/vault/version_manager/action_delete_branch_until_fork.pb.h"


namespace maidsafe {

namespace vault {

ActionDeleteBranchUntilFork::ActionDeleteBranchUntilFork(const std::string& serialised_action)
    : version_name_([&serialised_action]() {
                      protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
                      if (!action_delete_branch_until_fork_proto.ParseFromString(serialised_action))
                        ThrowError(CommonErrors::parsing_error);
                      return StructuredDataVersions::VersionName(
                          action_delete_branch_until_fork_proto.serialised_version_name());
                    }()) {}

ActionDeleteBranchUntilFork::ActionDeleteBranchUntilFork(
    const StructuredDataVersions::VersionName& version_name)
    : version_name_(version_name) {}

ActionDeleteBranchUntilFork::ActionDeleteBranchUntilFork(const ActionDeleteBranchUntilFork& other)
    : version_name(other.version_name) {}

ActionDeleteBranchUntilFork::ActionDeleteBranchUntilFork(ActionDeleteBranchUntilFork&& other)
    : version_name(std::move(other.version_name)) {}

std::string ActionDeleteBranchUntilFork::Serialise() const {
  protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
  action_delete_branch_until_fork_proto.set_serialised_version_name(version_name.Serialise());
  return action_delete_branch_until_fork_proto.SerializeAsString();
}

void ActionDeleteBranchUntilFork::operator()(boost::optional<VersionManagerValue> value) const {
  value->DeleteBranchUntilFork();
}

bool operator==(const ActionDeleteBranchUntilFork& lhs, const ActionDeleteBranchUntilFork& rhs) {
  return lhs.version_name == rhs.version_name;
}

bool operator!=(const ActionDeleteBranchUntilFork& lhs, const ActionDeleteBranchUntilFork& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
