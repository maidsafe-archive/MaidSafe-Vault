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

// void PmidManagerDispatcher::SendStateChange(const PmidName& pmid_node,
//                                            const Data::Name &data_name) {
//  typedef nfs::StateChangeFromPmidManagerToDataManager NfsMessage;
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
//  nfs::DataName data(nfs::DataName(pmid_node));
//  NfsMessage nfs_message(data);
//  RoutingMessage message(nfs_message.Serialise(),
//                         NfsMessage::Sender(routing::GroupId(pmid_node),
//                                            routing::SingleId(routing_.kNodeId())),
//                         NfsMessage::Receiver(NodeId(data_name->string())));
//  routing_.Send(message);
// }

// void PmidManagerDispatcher::SendAccountTransfer(const PmidName& destination_peer,
//                                                const PmidName& pmid_node,
//                                                const std::string& serialised_account) {
//  typedef nfs::AccountTransferFromPmidManagerToPmidManager NfsMessage;
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

//  NfsMessage nfs_message(serialised_account);  // TODO(Mahmoud): MUST BE FIXED
//  RoutingMessage message(nfs_message.Serialise(),
//                         NfsMessage::Sender(routing::GroupId(pmid_node),
//                                            routing::SingleId(routing_.kNodeId())),
//                         NfsMessage::Receiver(routing::GroupId(destination_peer)));
//  routing_.Send(message);
// }

void PmidManagerDispatcher::SendAccountTransfer(const NodeId& destination_peer,
                                                const PmidName& account_name,
                                                nfs::MessageId message_id,
                                                const std::string& serialised_account) {
  typedef AccountTransferFromPmidManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id, nfs_vault::Content(serialised_account));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(account_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::SingleId(destination_peer)));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendPmidAccount(const PmidName& pmid_node,
                                            const nfs_client::ReturnCode& return_code) {
  typedef GetPmidAccountResponseFromPmidManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message((nfs_client::ReturnCode(return_code)));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::SingleId(
                                                    NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendHealthResponse(const MaidName& maid_node,
    const PmidName& pmid_node, const PmidManagerMetadata& pmid_health,  nfs::MessageId message_id,
    const maidsafe_error& error) {
  LOG(kVerbose) << "PmidManagerDispatcher::SendHealthResponse for maid "
                << HexSubstr(maid_node->string()) << " and pmid " << HexSubstr(pmid_node->string())
                << " . PmidManagerMetadata serialised as " << HexSubstr(pmid_health.Serialise())
                << " and return code : " << boost::diagnostic_information(error);
  typedef PmidHealthResponseFromPmidManagerToMaidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, nfs_client::PmidHealthAndReturnCode(
                                             nfs_vault::PmidHealth(pmid_health.Serialise()),
                                             nfs_client::ReturnCode(error)));
  LOG(kVerbose) << "Send PmidHealthResponse to group around "
                << HexSubstr(maid_node.value.string());
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(maid_node.value.string())));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendHealthRequest(const PmidName& pmid_node,
                                              nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerDispatcher::SendHealthRequest to pmid "
                << HexSubstr(pmid_node->string()) << " with message_id " << message_id.data;
  typedef PmidHealthRequestFromPmidManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, nfs_vault::Empty());
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing_.kNodeId()),
                         VaultMessage::Receiver(NodeId(pmid_node->string())));
  routing_.Send(message);
}
/*
 * PmidManager no longer report the PmidNode status to DataManager to mark node up / down
void PmidManagerDispatcher::SendSetPmidOnline(const nfs_vault::DataName& data_name,
                                              const PmidName& pmid_node) {
  nfs::MessageId message_id(HashStringToMessageId(pmid_node->string() +
                                                  data_name.raw_name.string()));
  typedef SetPmidOnlineFromPmidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, data_name);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(data_name.raw_name.string())));
  routing_.Send(message);
}


void PmidManagerDispatcher::SendSetPmidOffline(const nfs_vault::DataName& data_name,
                                               const PmidName& pmid_node) {
  nfs::MessageId message_id(HashStringToMessageId(pmid_node->string() +
                                                  data_name.raw_name.string()));
  typedef SetPmidOfflineFromPmidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, data_name);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(data_name.raw_name.string())));
  routing_.Send(message);
}
*/
routing::GroupSource PmidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
