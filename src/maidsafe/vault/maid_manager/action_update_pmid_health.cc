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

#include "maidsafe/vault/maid_manager/action_update_pmid_health.h"

#include "maidsafe/vault/maid_manager/action_update_pmid_health.pb.h"

namespace maidsafe {

namespace vault {

ActionMaidManagerUpdatePmidHealth::ActionMaidManagerUpdatePmidHealth(const PmidName& pmid_name,
    const PmidManagerValue& pmid_health_in)
  : kPmidName(pmid_name), kPmidHealth(pmid_health_in) {}

ActionMaidManagerUpdatePmidHealth::ActionMaidManagerUpdatePmidHealth(
    const std::string& serialised_action)
        : kPmidHealth([&serialised_action]()->PmidManagerValue {
            protobuf::ActionMaidManagerUpdatePmidHealth action_proto;
            if (!action_proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return PmidManagerValue(action_proto.serialised_pmid_health());
          }()) {}

ActionMaidManagerUpdatePmidHealth::ActionMaidManagerUpdatePmidHealth(
    const ActionMaidManagerUpdatePmidHealth& other)
        : kPmidName(other.kPmidName), kPmidHealth(other.kPmidHealth) {}

ActionMaidManagerUpdatePmidHealth::ActionMaidManagerUpdatePmidHealth(
    ActionMaidManagerUpdatePmidHealth&& other)
        : kPmidName(other.kPmidName), kPmidHealth(std::move(other.kPmidHealth)) {}

std::string ActionMaidManagerUpdatePmidHealth::Serialise() const {
  protobuf::ActionMaidManagerUpdatePmidHealth action_proto;
  action_proto.set_serialised_pmid_name(kPmidName->string());
  action_proto.set_serialised_pmid_health(kPmidHealth.Serialise());
  return action_proto.SerializeAsString();
}

void ActionMaidManagerUpdatePmidHealth::operator()(MaidManagerMetadata& metadata) {
  metadata.UpdatePmidTotals(kPmidName, kPmidHealth);
}

bool operator==(const ActionMaidManagerUpdatePmidHealth& lhs,
                const ActionMaidManagerUpdatePmidHealth& rhs) {
  return (lhs.kPmidName == rhs.kPmidName && lhs.kPmidHealth == rhs.kPmidHealth);
}

bool operator!=(const ActionMaidManagerUpdatePmidHealth& lhs,
                const ActionMaidManagerUpdatePmidHealth& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
