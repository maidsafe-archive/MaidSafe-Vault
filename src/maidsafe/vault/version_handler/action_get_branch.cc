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

#include "maidsafe/vault/version_handler/action_get_branch.h"
#include "maidsafe/vault/version_handler/action_get_branch.pb.h"

namespace maidsafe {

namespace vault {

const nfs::MessageAction ActionVersionHandlerGetBranch::kActionId;

ActionVersionHandlerGetBranch::ActionVersionHandlerGetBranch(
    const std::string& serialised_action)
        : version_name([&serialised_action]() {
                         protobuf::ActionVersionHandlerGetBranch action_get_branch_proto;
                           if (!action_get_branch_proto.ParseFromString(serialised_action))
                             ThrowError(CommonErrors::parsing_error);
                         return StructuredDataVersions::VersionName(
                             action_get_branch_proto.serialised_version());
                    }())  {}

ActionVersionHandlerGetBranch::ActionVersionHandlerGetBranch(
    const StructuredDataVersions::VersionName& version_name_in)
    : version_name(version_name_in) {}

ActionVersionHandlerGetBranch::ActionVersionHandlerGetBranch(
  const ActionVersionHandlerGetBranch& other)
    : version_name(other.version_name) {}

ActionVersionHandlerGetBranch::ActionVersionHandlerGetBranch(ActionVersionHandlerGetBranch&& other)
    : version_name(std::move(other.version_name)) {}

 std::string ActionVersionHandlerGetBranch::Serialise() const {
  protobuf::ActionVersionHandlerGetBranch action_get_branch_proto;
  action_get_branch_proto.set_serialised_version(version_name.Serialise());
  return action_get_branch_proto.SerializeAsString();
}

void ActionVersionHandlerGetBranch::operator()(
    boost::optional<VersionHandlerValue>& /*value*/,
    std::vector<StructuredDataVersions::VersionName>& version_names) const {
  version_names.clear();
  //  if (value)
  //    version_names = value->GetBranch();
}

bool operator==(const ActionVersionHandlerGetBranch& lhs,
                const ActionVersionHandlerGetBranch& rhs) {
  return lhs.version_name == rhs.version_name;
}

bool operator!=(const ActionVersionHandlerGetBranch& lhs,
                const ActionVersionHandlerGetBranch& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
