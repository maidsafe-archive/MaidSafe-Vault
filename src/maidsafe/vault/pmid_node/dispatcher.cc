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

#include "maidsafe/vault/pmid_node/dispatcher.h"

namespace maidsafe {

namespace vault {

PmidNodeDispatcher::PmidNodeDispatcher(routing::Routing& routing) : routing_(routing) {}

void PmidNodeDispatcher::SendGetRequest(const nfs_vault::DataName& data_name) {
  typedef GetRequestFromPmidNodeToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  nfs_vault::DataName data(data_name);
  VaultMessage vault_message(data);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(data_name.raw_name)));
  routing_.Send(message);
}

void PmidNodeDispatcher::SendPmidAccountRequest() {
  typedef GetPmidAccountRequestFromPmidNodeToPmidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message;
  RoutingMessage message(vault_message.Serialise(), VaultMessage::Sender(routing_.kNodeId()),
                         VaultMessage::Receiver(routing_.kNodeId()));
  routing_.Send(message);
}

routing::GroupSource PmidNodeDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
