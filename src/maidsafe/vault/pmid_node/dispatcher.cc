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

#include "maidsafe/vault/pmid_node/dispatcher.h"

namespace maidsafe {

namespace vault {

PmidNodeDispatcher::PmidNodeDispatcher(routing::Routing& routing)
    : routing_(routing) {}

void PmidNodeDispatcher::SendGetRequest(const Data::Name &data_name) {
  typedef nfs::GetRequestFromPmidNodeToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  nfs::DataName data(nfs::DataName(data_name));
  NfsMessage nfs_message(data);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}

void PmidNodeDispatcher::SendAccountTransferRequest() {
  typedef nfs::GetPmidAccountRequestFromPmidNodeToPmidManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage nfs_message;
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(routing::GroupId(pmid_node)));
  routing_.Send(message);
}

routing::GroupSource PmidNodeDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
