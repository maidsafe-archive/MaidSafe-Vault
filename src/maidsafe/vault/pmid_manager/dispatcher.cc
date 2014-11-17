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

#include "maidsafe/vault/pmid_manager/dispatcher.h"

namespace maidsafe {

namespace vault {

PmidManagerDispatcher::PmidManagerDispatcher(routing::Routing& routing) : routing_(routing) {}

void PmidManagerDispatcher::SendAccountTransfer(const NodeId& peer,
                                                const std::string& serialised_account) {
  typedef AccountTransferFromPmidManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message{ nfs_vault::Content(serialised_account) };
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing_.kNodeId()),
                         VaultMessage::Receiver(routing::SingleId(peer)));
  routing_.Send(message);
}

routing::GroupSource PmidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

void PmidManagerDispatcher::SendAccountQuery(const PmidManager::Key& key) {
  typedef AccountQueryFromPmidManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message;
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing_.kNodeId()),
                         VaultMessage::Receiver(routing::GroupId(NodeId(key->string()))));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendAccountQueryResponse(const std::string& serialised_account,
                                                     const routing::GroupId& group_id,
                                                     const NodeId& sender) {
  typedef AccountQueryResponseFromPmidManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message{ VaultMessage::Contents{ serialised_account } };
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(group_id),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(sender));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
