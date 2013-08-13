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

namespace maidsafe {

namespace vault {

PmidManagerDispatcher::PmidManagerDispatcher(routing::Routing& routing)
    : routing_(routing) {}

void PmidManagerDispatcher::SendSync(const NodeId& destination_peer,
                                     const PmidName& account_name,
                                     const std::string& serialised_sync) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kSynchronise);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidManager);

  nfs::Message::Data inner_data;
  inner_data.content = NonEmptyString(serialised_sync);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(),
                         routing::GroupSource(routing::GroupId(account_name),
                                              routing::SingleId(routing_.kNodeId())),
                         routing::SingleId(destination_peer), cacheable);
  routing_.Send(message);
}

void PmidManagerDispatcher::SendAccountTransfer(const NodeId& destination_peer,
                                                const PmidName& account_name,
                                                const std::string& serialised_account) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kAccountTransfer);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidManager);

  nfs::Message::Data inner_data;
  inner_data.content = NonEmptyString(serialised_account);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(destination_peer), cacheable);
  routing_.Send(message);
}

routing::GroupSource PmidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
