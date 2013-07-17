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

#include "maidsafe/vault/version_manager/action_put_version.h"
#include "maidsafe/vault/version_manager/action_put_version.pb.h"


namespace maidsafe {

namespace vault {

const VersionManager::Action ActionPutVersion::action_id(VersionManager::Action::kPut);

ActionPutVersion::ActionPutVersion(const std::string& serialised_action)
    : old_version([&serialised_action]()->StructuredDataVersions::VersionName {
        protobuf::ActionPutVersion action_put_version_proto;
        if (!action_put_version_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return StructuredDataVersions::VersionName(action_put_version_proto.old_version().index(),
            ImmutableData::name_type(Identity(action_put_version_proto.old_version().id())));
      }()),
      new_version([&serialised_action]()->StructuredDataVersions::VersionName {
        protobuf::ActionPutVersion action_put_version_proto;
        action_put_version_proto.ParseFromString(serialised_action);
        return StructuredDataVersions::VersionName(action_put_version_proto.new_version().index(),
            ImmutableData::name_type(Identity(action_put_version_proto.new_version().id())));
      }()) {}

ActionPutVersion::ActionPutVersion(const ActionPutVersion& other)
    : old_version(other.old_version),
      new_version(other.new_version) {}

ActionPutVersion::ActionPutVersion(ActionPutVersion&& other)
    : old_version(std::move(other.old_version)),
      new_version(std::move(other.new_version)) {}

std::string ActionPutVersion::Serialise() const {
  protobuf::ActionPutVersion action_put_version_proto;
  action_put_version_proto.mutable_old_version()->set_index(old_version.index);
  action_put_version_proto.mutable_old_version()->set_id(old_version.id->string());
  action_put_version_proto.mutable_new_version()->set_index(new_version.index);
  action_put_version_proto.mutable_new_version()->set_id(new_version.id->string());
  return action_put_version_proto.SerializeAsString();
}

bool operator==(const ActionPutVersion& lhs, const ActionPutVersion& rhs) {
  return lhs.old_version == rhs.old_version &&
         lhs.new_version == rhs.new_version;
}

bool operator!=(const ActionPutVersion& lhs, const ActionPutVersion& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
