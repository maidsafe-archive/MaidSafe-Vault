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

#include "maidsafe/vault/mpid_manager/action_delete_alert.h"
#include "maidsafe/vault/mpid_manager/action_delete_alert.pb.h"

namespace maidsafe {

namespace vault {

ActionMpidManagerDeleteAlert::ActionMpidManagerDeleteAlert(
    const nfs_vault::MpidMessageAlert& alert) : kAlert(alert) {}

ActionMpidManagerDeleteAlert::ActionMpidManagerDeleteAlert(const std::string& serialised_action)
  : kAlert([&serialised_action]()->std::string {
            protobuf::ActionMpidManagerDeleteAlert proto;
            if (!proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return proto.serialised_alert ();
          }()) {}

ActionMpidManagerDeleteAlert::ActionMpidManagerDeleteAlert(
    const ActionMpidManagerDeleteAlert& other) : kAlert(other.kAlert) {}

ActionMpidManagerDeleteAlert::ActionMpidManagerDeleteAlert(ActionMpidManagerDeleteAlert&& other)
    : kAlert(std::move(other.kAlert)) {}

std::string ActionMpidManagerDeleteAlert::Serialise() const {
  protobuf::ActionMpidManagerDeleteAlert proto;
  proto.set_serialised_alert(kAlert.Serialise());
  return proto.SerializeAsString();
}

bool operator==(const ActionMpidManagerDeleteAlert& lhs, const ActionMpidManagerDeleteAlert& rhs) {
  return lhs.kAlert == rhs.kAlert;
}

bool operator!=(const ActionMpidManagerDeleteAlert& lhs, const ActionMpidManagerDeleteAlert& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
