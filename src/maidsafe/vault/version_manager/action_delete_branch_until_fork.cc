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

#include "maidsafe/vault/version_manager/action_delete_branch_until_fork.h"
#include "maidsafe/vault/version_manager/action_delete_branch_until_fork.pb.h"

namespace maidsafe {

namespace vault {

const nfs::MessageAction ActionVersionManagerDeleteBranchUntilFork::kActionId;

ActionVersionManagerDeleteBranchUntilFork::ActionVersionManagerDeleteBranchUntilFork(
    const std::string& serialised_action)
    : version_name([&serialised_action]() {
        protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
        if (!action_delete_branch_until_fork_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return StructuredDataVersions::VersionName(
            action_delete_branch_until_fork_proto.serialised_version());
      }()) {}

ActionVersionManagerDeleteBranchUntilFork::ActionVersionManagerDeleteBranchUntilFork(
    const StructuredDataVersions::VersionName& version_name_in)
    : version_name(version_name_in) {}

ActionVersionManagerDeleteBranchUntilFork::ActionVersionManagerDeleteBranchUntilFork(
    const ActionVersionManagerDeleteBranchUntilFork& other)
    : version_name(other.version_name) {}

ActionVersionManagerDeleteBranchUntilFork::ActionVersionManagerDeleteBranchUntilFork(
    const ActionVersionManagerDeleteBranchUntilFork&& other)
    : version_name(std::move(other.version_name)) {}

std::string ActionVersionManagerDeleteBranchUntilFork::Serialise() const {
  protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
  action_delete_branch_until_fork_proto.set_serialised_version(version_name.Serialise());
  return action_delete_branch_until_fork_proto.SerializeAsString();
}

void ActionVersionManagerDeleteBranchUntilFork::operator()(
    boost::optional<VersionManagerValue>& value) const {
  if (!value)
    ThrowError(CommonErrors::uninitialised);
  value->DeleteBranchUntilFork(version_name);
}

bool operator==(const ActionVersionManagerDeleteBranchUntilFork& lhs,
                const ActionVersionManagerDeleteBranchUntilFork& rhs) {
  return lhs.version_name == rhs.version_name;
}

bool operator!=(const ActionVersionManagerDeleteBranchUntilFork& lhs,
                const ActionVersionManagerDeleteBranchUntilFork& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
