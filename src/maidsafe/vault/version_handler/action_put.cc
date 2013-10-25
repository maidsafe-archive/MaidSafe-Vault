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

#include "maidsafe/vault/version_handler/action_put.h"
#include "maidsafe/vault/version_handler/action_put.pb.h"

#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

const nfs::MessageAction ActionVersionHandlerPut::kActionId;

ActionVersionHandlerPut::ActionVersionHandlerPut(const std::string& serialised_action) {
  protobuf::ActionPut action_put_version_proto;
  if (!action_put_version_proto.ParseFromString(serialised_action))
    ThrowError(CommonErrors::parsing_error);
    old_version = StructuredDataVersions::VersionName(
                      action_put_version_proto.serialised_old_version());
    new_version = StructuredDataVersions::VersionName(
                      action_put_version_proto.serialised_new_version());
}

ActionVersionHandlerPut::ActionVersionHandlerPut(const ActionVersionHandlerPut& other)
    : old_version(other.old_version), new_version(other.new_version) {}

ActionVersionHandlerPut::ActionVersionHandlerPut(ActionVersionHandlerPut&& other)
    : old_version(std::move(other.old_version)), new_version(std::move(other.new_version)) {}

std::string ActionVersionHandlerPut::Serialise() const {
  protobuf::ActionPut action_put_version_proto;
  action_put_version_proto.set_serialised_old_version(old_version.Serialise());
  action_put_version_proto.set_serialised_new_version(new_version.Serialise());
  return action_put_version_proto.SerializeAsString();
}

void ActionVersionHandlerPut::operator()(boost::optional<VersionHandlerValue>& value) const {
  if (!value) {
    value.reset();
  }
  value->Put(old_version, new_version);
}

bool operator==(const ActionVersionHandlerPut& lhs, const ActionVersionHandlerPut& rhs) {
  return lhs.old_version == rhs.old_version && lhs.new_version == rhs.new_version;
}

bool operator!=(const ActionVersionHandlerPut& lhs, const ActionVersionHandlerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
