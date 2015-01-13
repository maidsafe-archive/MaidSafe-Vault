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

#include "maidsafe/vault/version_handler/action_delete_branch_until_fork.h"

#include "maidsafe/common/serialisation/serialisation.h"

#include "maidsafe/vault/version_handler/action_delete_branch_until_fork.pb.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

ActionVersionHandlerDeleteBranchUntilFork::ActionVersionHandlerDeleteBranchUntilFork(
    const std::string& serialised_action)
    : version_name([&serialised_action]() -> StructuredDataVersions::VersionName {
        protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
        if (!action_delete_branch_until_fork_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return ConvertFromString<StructuredDataVersions::VersionName>(
            action_delete_branch_until_fork_proto.serialised_version());
      }()) {}

ActionVersionHandlerDeleteBranchUntilFork::ActionVersionHandlerDeleteBranchUntilFork(
    const StructuredDataVersions::VersionName& version_name_in)
    : version_name(version_name_in) {}

ActionVersionHandlerDeleteBranchUntilFork::ActionVersionHandlerDeleteBranchUntilFork(
    const ActionVersionHandlerDeleteBranchUntilFork& other)
        : version_name(other.version_name) {}

ActionVersionHandlerDeleteBranchUntilFork::ActionVersionHandlerDeleteBranchUntilFork(
    const ActionVersionHandlerDeleteBranchUntilFork&& other)
        : version_name(std::move(other.version_name)) {}

std::string ActionVersionHandlerDeleteBranchUntilFork::Serialise() const {
  protobuf::ActionDeleteBranchUntilFork action_delete_branch_until_fork_proto;
  action_delete_branch_until_fork_proto.set_serialised_version(ConvertToString(version_name));
  return action_delete_branch_until_fork_proto.SerializeAsString();
}

detail::DbAction ActionVersionHandlerDeleteBranchUntilFork::operator()(
    std::unique_ptr<VersionHandlerValue>& value) {
  if (!value) {
    LOG(kError) << "ActionVersionHandlerDeleteBranchUntilFork::operator() value uninitialised";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
  }
  value->DeleteBranchUntilFork(version_name);
  return detail::DbAction::kPut;
}

bool operator==(const ActionVersionHandlerDeleteBranchUntilFork& lhs,
                const ActionVersionHandlerDeleteBranchUntilFork& rhs) {
  return lhs.version_name == rhs.version_name;
}

bool operator!=(const ActionVersionHandlerDeleteBranchUntilFork& lhs,
                const ActionVersionHandlerDeleteBranchUntilFork& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
