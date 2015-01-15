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

#include "maidsafe/vault/message_types.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/mpid_manager/dispatcher.h"

namespace maidsafe {

namespace vault {

MpidManagerDispatcher::MpidManagerDispatcher(routing::Routing& routing) : routing_(routing) {}

void MpidManagerDispatcher::SendMessageAlert(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& sender, const MpidName& receiver) {
  using  VaultMessage = MessageAlertFromMpidManagerToMpidManager;
  CheckSourcePersonaType<VaultMessage>();
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  VaultMessage::Contents vault_message(alert);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(sender->string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendMessageAlert(const nfs_vault::MpidMessageAlert& alert,
                                             const MpidName& receiver) {
  using  NfstMessage = nfs::MessageAlertFromMpidManagerToMpidNode;
  CheckSourcePersonaType<NfstMessage>();
  using RoutingMessage = routing::Message<NfstMessage::Sender, NfstMessage::Receiver>;
  NfstMessage::Contents nfs_message(alert);
  RoutingMessage message(nfs_message.Serialise(),
                         NfstMessage::Sender(routing::GroupId(NodeId(receiver->string())),
                                              routing::SingleId(routing_.kNodeId())),
                         NfstMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendGetMessageRequest(const nfs_vault::MpidMessageAlert& alert,
                                                  const MpidName& receiver) {
  using  VaultMessage = GetRequestFromMpidManagerToMpidManager;
  CheckSourcePersonaType<VaultMessage>();
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  VaultMessage::Contents vault_message(alert);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(receiver->string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(alert.sender->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendGetMessageResponse(const DbMessageQueryResult& query_result,
                                                   const MpidName& sender,
                                                   const MpidName& receiver) {
  using  VaultMessage = GetResponseFromMpidManagerToMpidManager;
  CheckSourcePersonaType<VaultMessage>();
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  VaultMessage::Contents vault_message(query_result);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(sender->string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendGetMessageResponseToMpid(
    const nfs_client::MpidMessageOrReturnCode& response, const MpidName& receiver) {
  using NfsMessage = nfs::GetResponseFromMpidManagerToMpidNode;
  CheckSourcePersonaType<NfsMessage>();
  using RoutingMessage = routing::Message<NfsMessage::Sender, NfsMessage::Receiver>;
  NfsMessage::Contents nfs_message(response);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(receiver->string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendDeleteRequest(const nfs_vault::MpidMessageAlert& alert,
                                              const MpidName& receiver) {
  using  VaultMessage = DeleteRequestFromMpidManagerToMpidManager;
  CheckSourcePersonaType<VaultMessage>();
  using RoutingMessage = routing::Message<VaultMessage::Sender, VaultMessage::Receiver>;
  VaultMessage::Contents vault_message(alert);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(alert.sender->string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

void MpidManagerDispatcher::SendMessageResponse(const MpidName& receiver,
                                                const maidsafe_error& error) {
  using NfsMessage = nfs::SendMessageResponseFromMpidManagerToMpidNode;
  CheckSourcePersonaType<NfsMessage>();
  using RoutingMessage = routing::Message<NfsMessage::Sender, NfsMessage::Receiver>;
  NfsMessage::Contents nfs_message(error);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(receiver->string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(receiver->string())));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
