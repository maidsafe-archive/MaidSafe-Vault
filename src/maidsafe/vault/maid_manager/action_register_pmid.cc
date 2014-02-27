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

#include "maidsafe/vault/maid_manager/action_register_pmid.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/maid_manager/action_register_pmid.pb.h"
#include "maidsafe/vault/maid_manager/metadata.h"

namespace maidsafe {

namespace vault {

ActionMaidManagerRegisterPmid::ActionMaidManagerRegisterPmid(
    const nfs_vault::PmidRegistration& pmid_registration_in, nfs::MessageId message_id_in)
        : pmid_registration(pmid_registration_in),
          message_id(message_id_in) {}

ActionMaidManagerRegisterPmid::ActionMaidManagerRegisterPmid(
    const std::string& serialised_action)
        : pmid_registration(), message_id() {
  protobuf::ActionMaidManagerRegisterPmid action_register_pmid_proto;
  if (!action_register_pmid_proto.ParseFromString(serialised_action))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
   pmid_registration = nfs_vault::PmidRegistration(
                           action_register_pmid_proto.serialised_pmid_registration());
   message_id = nfs::MessageId(action_register_pmid_proto.message_id());
}

ActionMaidManagerRegisterPmid::ActionMaidManagerRegisterPmid(
    const ActionMaidManagerRegisterPmid& other)
    : pmid_registration(other.pmid_registration), message_id(other.message_id) {}

ActionMaidManagerRegisterPmid::ActionMaidManagerRegisterPmid(
    ActionMaidManagerRegisterPmid&& other)
    : pmid_registration(std::move(other.pmid_registration)),
      message_id(std::move(other.message_id)) {}

std::string ActionMaidManagerRegisterPmid::Serialise() const {
  protobuf::ActionMaidManagerRegisterPmid action_register_pmid_proto;
  action_register_pmid_proto.set_serialised_pmid_registration(pmid_registration.Serialise());
  action_register_pmid_proto.set_message_id(message_id);
  return action_register_pmid_proto.SerializeAsString();
}

bool operator==(const ActionMaidManagerRegisterPmid& lhs,
                const ActionMaidManagerRegisterPmid& rhs) {
  return lhs.pmid_registration.maid_name() == rhs.pmid_registration.maid_name() &&
         lhs.pmid_registration.pmid_name() == rhs.pmid_registration.pmid_name() &&
         lhs.pmid_registration.unregister() == rhs.pmid_registration.unregister() &&
         lhs.message_id == rhs.message_id;
}

bool operator!=(const ActionMaidManagerRegisterPmid& lhs,
                const ActionMaidManagerRegisterPmid& rhs) {
  return !operator==(lhs, rhs);
}

void ActionMaidManagerRegisterPmid::operator()(
    MaidManagerMetadata& metadata) const {
  metadata.RegisterPmid(pmid_registration);
}

}  // namespace vault

}  // namespace maidsafe
