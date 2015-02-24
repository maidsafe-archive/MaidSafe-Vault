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

void PmidNodeDispatcher::SendGetOrIntegrityCheckResponse(
    const nfs_vault::DataNameAndContentOrCheckResult& data_or_check_result,
    const NodeId& data_manager_node_id,
    nfs::MessageId message_id) {
  typedef GetResponseFromPmidNodeToDataManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id, data_or_check_result);
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::SingleId(data_manager_node_id)));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
