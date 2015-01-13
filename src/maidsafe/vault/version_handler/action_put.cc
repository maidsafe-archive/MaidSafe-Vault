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

#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/common/serialisation/serialisation.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/version_handler/action_put.pb.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

ActionVersionHandlerPut::ActionVersionHandlerPut(
    const StructuredDataVersions::VersionName& old_version_in,
    const StructuredDataVersions::VersionName& new_version_in,
    const Identity& originator_in,
    nfs::MessageId message_id_in)
        : old_version(old_version_in), new_version(new_version_in), tip_of_tree(),
          message_id(message_id_in), originator(originator_in) {}

ActionVersionHandlerPut::ActionVersionHandlerPut(const std::string& serialised_action) {
  protobuf::ActionPut action_put_version_proto;
  if (!action_put_version_proto.ParseFromString(serialised_action))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
    if (action_put_version_proto.has_serialised_old_version())
      old_version = ConvertFromString<StructuredDataVersions::VersionName>(
                        action_put_version_proto.serialised_old_version());
    new_version = ConvertFromString<StructuredDataVersions::VersionName>(
                      action_put_version_proto.serialised_new_version());
    message_id = nfs::MessageId(action_put_version_proto.message_id());
    originator = Identity(action_put_version_proto.originator());
}

ActionVersionHandlerPut::ActionVersionHandlerPut(const ActionVersionHandlerPut& other)
    : old_version(other.old_version), new_version(other.new_version),
      tip_of_tree(other.tip_of_tree), message_id(other.message_id),
      originator(other.originator) {}

ActionVersionHandlerPut::ActionVersionHandlerPut(ActionVersionHandlerPut&& other)
    : old_version(std::move(other.old_version)), new_version(std::move(other.new_version)),
      tip_of_tree(std::move(other.tip_of_tree)), message_id(std::move(other.message_id)),
      originator(std::move(other.originator)) {}

std::string ActionVersionHandlerPut::Serialise() const {
  protobuf::ActionPut action_put_version_proto;
  if (old_version.id->IsInitialised())
    action_put_version_proto.set_serialised_old_version(ConvertToString(old_version));
  action_put_version_proto.set_serialised_new_version(ConvertToString(new_version));
  action_put_version_proto.set_message_id(message_id.data);
  action_put_version_proto.set_originator(originator.string());
  return action_put_version_proto.SerializeAsString();
}

detail::DbAction ActionVersionHandlerPut::operator()(std::unique_ptr<VersionHandlerValue>& value) {
  if (!value)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  VLOG(nfs::Persona::kVersionHandler, VisualiserAction::kChangeVersion, old_version.id.value,
       new_version.id.value);
  tip_of_tree = value->Put(old_version, new_version);
  return detail::DbAction::kPut;
}

bool operator==(const ActionVersionHandlerPut& lhs, const ActionVersionHandlerPut& rhs) {
  return lhs.old_version == rhs.old_version && lhs.new_version == rhs.new_version &&
         lhs.message_id == rhs.message_id && lhs.originator == rhs.originator;
}

bool operator!=(const ActionVersionHandlerPut& lhs, const ActionVersionHandlerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
