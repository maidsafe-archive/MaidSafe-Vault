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

#include "maidsafe/vault/version_manager/action_put.h"
#include "maidsafe/vault/version_manager/action_put_version.pb.h"


namespace maidsafe {

namespace vault {

const nfs::MessageAction ActionVersionManagerPut::kActionId;

ActionVersionManagerPut::ActionVersionManagerPut(
    const std::string& serialised_action) {
  protobuf::ActionVersionManagerPut action_put_version_proto;
  if (!action_put_version_proto.ParseFromString(serialised_action))
    ThrowError(CommonErrors::parsing_error);
  old_version = StructuredDataVersions::VersionName(
                    action_put_version_proto.serialised_old_version);
  new_version = StructuredDataVersions::VersionName(
                    action_put_version_proto.serialised_new_version);
}

ActionVersionManagerPut::ActionVersionManagerPut(const ActionVersionManagerPut& other)
    : old_version(other.old_version),
      new_version(other.new_version) {}

ActionVersionManagerPut::ActionVersionManagerPut(ActionVersionManagerPut&& other)
    : old_version(std::move(other.old_version)),
      new_version(std::move(other.new_version)) {}

std::string ActionVersionManagerPut::Serialise() const {
  protobuf::ActionVersionManagerPut action_put_version_proto;
  action_put_version_proto.set_serialised_old_version(old_version.Serialise());
  action_put_version_proto.set_serialised_new_version(new_version.Serialise());
  return action_put_version_proto.SerializeAsString();
}

void ActionVersionManagerPut::operator()(boost::optional<VersionManagerValue>& value) const {
  if (!value) {
    value.reset(VersionManagerValue());
  }
  value->Put(old_version, new_version);
}

bool operator==(const ActionVersionManagerPut& lhs, const ActionVersionManagerPut& rhs) {
  return lhs.old_version == rhs.old_version &&
         lhs.new_version == rhs.new_version;
}

bool operator!=(const ActionVersionManagerPut& lhs, const ActionVersionManagerPut& rhs) {
  return !operator==(lhs, rhs);
}



}  // namespace vault

}  // namespace maidsafe
