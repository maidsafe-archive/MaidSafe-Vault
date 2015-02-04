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

#include "maidsafe/vault/version_handler/action_create_version_tree.h"
#include "maidsafe/vault/version_handler/action_create_version_tree.pb.h"

#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

ActionVersionHandlerCreateVersionTree::ActionVersionHandlerCreateVersionTree(
    const StructuredDataVersions::VersionName& version_in, const Identity& originator_in,
    uint32_t max_versions_in, uint32_t max_branches_in, nfs::MessageId message_id_in)
        : version(version_in), max_versions(max_versions_in),
          max_branches(max_branches_in), message_id(message_id_in), originator(originator_in) {}

ActionVersionHandlerCreateVersionTree::ActionVersionHandlerCreateVersionTree(
    const std::string& serialised_action) {
  protobuf::ActionCreateVersionTree action_create_version_proto;
  if (!action_create_version_proto.ParseFromString(serialised_action))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  version = StructuredDataVersions::VersionName(
      ConvertFromString<StructuredDataVersions::VersionName>(
          action_create_version_proto.serialised_version()));
  max_versions = action_create_version_proto.max_versions();
  max_branches = action_create_version_proto.max_branches();
  message_id = nfs::MessageId(action_create_version_proto.message_id());
  originator = Identity(action_create_version_proto.originator());
}

ActionVersionHandlerCreateVersionTree::ActionVersionHandlerCreateVersionTree(
    const ActionVersionHandlerCreateVersionTree& other)
        : version(other.version), max_versions(other.max_versions),
          max_branches(other.max_branches), message_id(other.message_id),
          originator(other.originator) {}

ActionVersionHandlerCreateVersionTree::ActionVersionHandlerCreateVersionTree(
    ActionVersionHandlerCreateVersionTree&& other)
        : version(std::move(other.version)), max_versions(std::move(other.max_versions)),
          max_branches(std::move(other.max_branches)), message_id(std::move(other.message_id)),
          originator(std::move(other.originator)) {}

std::string ActionVersionHandlerCreateVersionTree::Serialise() const {
  protobuf::ActionCreateVersionTree action_create_version_proto;
  action_create_version_proto.set_serialised_version(ConvertToString(version));
  action_create_version_proto.set_max_versions(max_versions);
  action_create_version_proto.set_max_branches(max_branches);
  action_create_version_proto.set_message_id(message_id.data);
  action_create_version_proto.set_originator(originator.string());
  return action_create_version_proto.SerializeAsString();
}

detail::DbAction ActionVersionHandlerCreateVersionTree::operator()(
    std::unique_ptr<VersionHandlerValue>& value) {
  if (value)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::data_already_exists));
  value.reset(new VersionHandlerValue(max_versions, max_branches));
  value->Put(StructuredDataVersions::VersionName(), version);
  return detail::DbAction::kPut;
}

bool operator==(const ActionVersionHandlerCreateVersionTree& lhs,
                const ActionVersionHandlerCreateVersionTree& rhs) {
  return lhs.version == rhs.version && lhs.max_versions == rhs.max_versions &&
         lhs.max_branches == rhs.max_branches && lhs.message_id == rhs.message_id &&
         lhs.originator == rhs.originator;
}

bool operator!=(const ActionVersionHandlerCreateVersionTree& lhs,
                const ActionVersionHandlerCreateVersionTree& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
