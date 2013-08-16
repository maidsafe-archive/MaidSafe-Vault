/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/pmid_manager/dispatcher.h"

#include "message_types.h"

namespace maidsafe {

namespace vault {

PmidManagerDispatcher::PmidManagerDispatcher(routing::Routing& routing)
    : routing_(routing) {}

void PmidManagerDispatcher::SendStateChange(const PmidName& pmid_node,
                                            const Data::Name &data_name) {
  typedef nfs::StateChangeFromPmidManagerToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  nfs::DataName data(nfs::DataName(pmid_node));
  NfsMessage nfs_message(data);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(pmid_node),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendSync(const PmidName& pmid_node,
                                     const std::string& serialised_sync) {
  typedef nfs::SynchroniseFromPmidManagerToPmidManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage nfs_message(serialised_sync); // TODO(Mahmoud): MUST BE FIXED
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(pmid_node),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(routing::GroupId(pmid_node)));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendAccountTransfer(const PmidName& destination_peer,
                                                const PmidName& pmid_node,
                                                const std::string& serialised_account) {
  typedef nfs::AccountTransferFromPmidManagerToPmidManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage nfs_message(serialised_account);  // TODO(Mahmoud): MUST BE FIXED
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(pmid_node),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(routing::GroupId(destination_peer)));
  routing_.Send(message);
}

void PmidManagerDispatcher::SendPmidAccount(const PmidName& pmid_node,
                                            const std::string& serialised_account_response) {
  typedef GetPmidAccountResponseFromPmidManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  PmidAccountResponse pmid_account_response(serialised_account_response);
  VaultMessage vault_message(pmid_account_response);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::SingleSource(
                                                  routing::SingleId(routing_.kNodeId()))),
                         VaultMessage::Receiver(routing::SingleId(pmid_node)));
  routing_.Send(message);
}

routing::GroupSource PmidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
