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

#include "maidsafe/vault/message_types.h"

namespace maidsafe {

namespace vault {

PmidManagerDispatcher::PmidManagerDispatcher(routing::Routing& routing)
    : routing_(routing) {}

//void PmidManagerDispatcher::SendStateChange(const PmidName& pmid_node,
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
//}

//void PmidManagerDispatcher::SendSync(const PmidName& pmid_node,
//                                     const std::string& serialised_sync) {
//  typedef nfs::SynchroniseFromPmidManagerToPmidManager NfsMessage;
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

//  NfsMessage nfs_message(serialised_sync); // TODO(Mahmoud): MUST BE FIXED
//  RoutingMessage message(nfs_message.Serialise(),
//                         NfsMessage::Sender(routing::GroupId(pmid_node),
//                                            routing::SingleId(routing_.kNodeId())),
//                         NfsMessage::Receiver(routing::GroupId(pmid_node)));
//  routing_.Send(message);
//}

//void PmidManagerDispatcher::SendAccountTransfer(const PmidName& destination_peer,
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
//}

//void PmidManagerDispatcher::SendPmidAccount(const PmidName& pmid_node,
//                                            const std::string& serialised_account_response) {
//  typedef GetPmidAccountResponseFromPmidManagerToPmidNode VaultMessage;
//  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

//  PmidAccountResponse pmid_account_response(serialised_account_response);
//  VaultMessage vault_message(pmid_account_response);
//  RoutingMessage message(vault_message.Serialise(),
//                         VaultMessage::Sender(routing::SingleSource(
//                                                  routing::SingleId(routing_.kNodeId()))),
//                         VaultMessage::Receiver(routing::SingleId(pmid_node)));
//  routing_.Send(message);
//}

routing::GroupSource PmidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
