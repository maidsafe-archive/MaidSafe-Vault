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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

class OwnerDirectory;
class GroupDirectory;
class WorldDirectory;

namespace vault {

class PmidManagerDispatcher {
 public:
  PmidManagerDispatcher(routing::Routing& routing);

  template<typename Data>
  void SendPutRequest(const Data& data, const PmidName& pmid_node);


  void SendSync(const NodeId& destination_peer,
                const PmidName& account_name,
                const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer,
                           const PmidName& account_name,
                           const std::string& serialised_account);

 private:
  PmidManagerDispatcher();
  PmidManagerDispatcher(const PmidManagerDispatcher&);
  PmidManagerDispatcher(PmidManagerDispatcher&&);
  PmidManagerDispatcher& operator=(PmidManagerDispatcher);

  routing::GroupSource Sender(const MaidName& account_name) const;

  routing::Routing& routing_;
};

// ==================== Implementation =============================================================

template<typename Data>
void PmidManagerDispatcher::SendPutRequest(const Data& data, const PmidName& pmid_node) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(is_cacheable<Data>::value ? routing::Cacheable::kGet :
                                                                        routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kPmidNode);
  static const DataTagValue kDataEnumValue(Data::Tag::kValue);

  nfs::Message::Data inner_data(kDataEnumValue, data.name().data, data.Serialise().data, kAction);
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data, pmid_node_hint);
  RoutingMessage message(inner.Serialise()->string(),
                         routing::GroupSource(routing::GroupId(pmid_node),
                                              routing::SingleId(routing_.kNodeId())),
                         routing::GroupId(NodeId(data.name()->string())), cacheable);
  routing_.Send(message);
}


template<typename Data>
void PmidManagerDispatcher::SendDeleteRequest(const PmidName& pmid_node,
                                              const typename Data::Name& data_name) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kDeleteRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kPmidNode);
  static const DataTagValue kDataEnumValue(Data::Tag::kValue);

  nfs::Message::Data inner_data(kDataEnumValue, data_name.data, NonEmptyString(), kAction);
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(),
                         routing::GroupSource(routing::GroupId(pmid_node),
                                              routing::SingleId(routing_.kNodeId())),
                         routing::GroupId(NodeId(data_name->string())), cacheable);
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
