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

#include "maidsafe/vault/mpid_manager/action_put_message.h"
#include "maidsafe/vault/mpid_manager/action_put_message.pb.h"

#include "maidsafe/vault/mpid_manager/value.h"

namespace maidsafe {

namespace vault {

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(
    const nfs_vault::MpidMessage& message) : kMessage(message) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(const std::string& serialised_action)
  : kMessage([&serialised_action]()->std::string {
            protobuf::ActionMpidManagerPutMessage proto;
            if (!proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return proto.serialised_message ();
          }()) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(
    const ActionMpidManagerPutMessage& other) : kMessage(other.kMessage) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(ActionMpidManagerPutMessage&& other)
    : kMessage(std::move(other.kMessage)) {}

std::string ActionMpidManagerPutMessage::Serialise() const {
  protobuf::ActionMpidManagerPutMessage proto;
  proto.set_serialised_message(kMessage.Serialise());
  return proto.SerializeAsString();
}

void ActionMpidManagerPutMessage::operator()(MpidManagerValue& value) {
  value.AddMessage(kMessage);
}

bool operator==(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs) {
  return lhs.kMessage == rhs.kMessage;
}

bool operator!=(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
