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

namespace maidsafe {

namespace vault {

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(
    const nfs_vault::MpidMessage& message, nfs::MessageId message_id)
        : kMessageAndId(MessageAndId(message, message_id)) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(const std::string& serialised_action)
  : kMessageAndId([&serialised_action]()->MessageAndId {
            protobuf::ActionMpidManagerPutMessage proto;
            if (!proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return MessageAndId(nfs_vault::MpidMessage(proto.serialised_message()),
                                nfs::MessageId(proto.message_id()));
          }()) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(
    const ActionMpidManagerPutMessage& other) : kMessageAndId(other.kMessageAndId) {}

ActionMpidManagerPutMessage::ActionMpidManagerPutMessage(ActionMpidManagerPutMessage&& other)
    : kMessageAndId(std::move(other.kMessageAndId)) {}

std::string ActionMpidManagerPutMessage::Serialise() const {
  protobuf::ActionMpidManagerPutMessage proto;
  proto.set_serialised_message(kMessageAndId.message.Serialise());
  proto.set_message_id(kMessageAndId.id.data);
  return proto.SerializeAsString();
}

bool operator==(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs) {
  return (lhs.kMessageAndId.message == rhs.kMessageAndId.message) &&
         (lhs.kMessageAndId.id == rhs.kMessageAndId.id);
}

bool operator!=(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
