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

#include "maidsafe/vault/data_manager/dispatcher.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {


namespace detail {

routing::SingleIdRelay GetDestination(
        const PartialRequestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>& requestor) {
  return routing::SingleIdRelay(routing::SingleId(NodeId(requestor.relay_source.node_id->string())),
      requestor.relay_source.connection_id,
                       routing::SingleId(NodeId(requestor.relay_source.relay_node->string())));
}

routing::SingleIdRelay GetDestination(
        const PartialRequestor<nfs::SourcePersona<nfs::Persona::kMpidNode>>& requestor) {
  return routing::SingleIdRelay(routing::SingleId(NodeId(requestor.relay_source.node_id->string())),
      requestor.relay_source.connection_id,
                       routing::SingleId(NodeId(requestor.relay_source.relay_node->string())));
}

routing::SingleIdRelay GetDestination(
        const PartialRequestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>& requestor) {
  return routing::SingleIdRelay(routing::SingleId(NodeId(requestor.relay_source.node_id->string())),
      requestor.relay_source.connection_id,
                       routing::SingleId(NodeId(requestor.relay_source.relay_node->string())));
}

// FIXME(Prakash) after changing requestor in vaults to hold exact sender type
routing::SingleId GetDestination(
        const Requestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>& requestor) {
  return routing::SingleId(requestor.node_id);
}

routing::SingleId GetDestination(
        const Requestor<nfs::SourcePersona<nfs::Persona::kMpidNode>>& requestor) {
  return routing::SingleId(requestor.node_id);
}

routing::SingleId GetDestination(
        const Requestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>& requestor) {
  return routing::SingleId(requestor.node_id);
}

}  // namespace detail

// ==================== Sync / AccountTransfer implementation ======================================
void DataManagerDispatcher::SendSync(const DataManager::Key& key,
                                     const std::string& serialised_sync) {
  typedef SynchroniseFromDataManagerToDataManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  SendSyncMessage<VaultMessage> sync_sender;
  sync_sender(routing_, VaultMessage((nfs_vault::Content(serialised_sync))), key);
}

void DataManagerDispatcher::SendAccountTransfer(const NodeId& destination_peer,
                                                const std::string& serialised_account) {
  typedef AccountTransferFromDataManagerToDataManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message((nfs_vault::Content(serialised_account)));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::SingleId(destination_peer)));
  routing_.Send(message);
}

void DataManagerDispatcher::SendAccountRequest(const Key& key) {
  typedef AccountQueryFromDataManagerToDataManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(VaultMessage::Contents(key.type, key.name));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing_.kNodeId()),
                         VaultMessage::Receiver(routing::GroupId(NodeId(key.name.string()))));
  routing_.Send(message);
}

void DataManagerDispatcher::SendAccountResponse(const std::string& serialised_account,
                                                const routing::GroupId& group_id,
                                                const NodeId& sender) {
  typedef AccountQueryResponseFromDataManagerToDataManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage::Contents content((serialised_account));
  VaultMessage vault_message(content);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(group_id),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(sender));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
