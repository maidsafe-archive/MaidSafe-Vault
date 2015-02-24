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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_ACTION_PUT_MESSAGE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_ACTION_PUT_MESSAGE_H_

#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {

struct ActionMpidManagerPutMessage {
  explicit ActionMpidManagerPutMessage(const nfs_vault::MpidMessage& message,
                                       nfs::MessageId message_id);
  explicit ActionMpidManagerPutMessage(const std::string& serialised_action);
  ActionMpidManagerPutMessage(const ActionMpidManagerPutMessage& other);
  ActionMpidManagerPutMessage(ActionMpidManagerPutMessage&& other);
  ActionMpidManagerPutMessage() = delete;
  ActionMpidManagerPutMessage& operator=(ActionMpidManagerPutMessage other) = delete;

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kSendMessageRequest;

  struct MessageAndId {
    MessageAndId(const nfs_vault::MpidMessage& message_in, nfs::MessageId id_in)
        : message(message_in), id(id_in) {}

    MessageAndId(const MessageAndId& other)
        : message(other.message), id(other.id) {}

    MessageAndId(MessageAndId&& other)
        : message(std::move(other.message)), id(std::move(other.id)) {}

    nfs_vault::MpidMessage message;
    nfs::MessageId id;
  };

  MessageAndId kMessageAndId;
};

bool operator==(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs);
bool operator!=(const ActionMpidManagerPutMessage& lhs, const ActionMpidManagerPutMessage& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_ACTION_PUT_MESSAGE_H_
