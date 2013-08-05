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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_

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

class MaidManagerDispatcher {
 public:
  MaidManagerDispatcher(routing::Routing& routing, const passport::Pmid& signing_fob);

  template<typename Data>
  void SendGetVersionRequest(const MaidName& account_name,
                             const typename Data::name_type& data_name);

  template<typename Data>
  void SendPutRequest(const MaidName& account_name,
                      const Data& data,
                      const PmidName& pmid_node_hint);

  template<typename Data>
  void SendPutResponse(const MaidName& account_name,
                       const typename Data::name_type& data_name,
                       const maidsafe_error& result);

  template<typename Data>
  void SendDeleteRequest(const MaidName& account_name, const typename Data::name_type& data_name);

  void SendCreateAccountResponse(const MaidName& account_name, const maidsafe_error& result);

  void SendRemoveAccountResponse(const MaidName& account_name, const maidsafe_error& result);

  void SendRegisterPmidResponse(const MaidName& account_name,
                                const PmidName& pmid_name,
                                const maidsafe_error& result);

  void SendUnregisterPmidResponse(const MaidName& account_name,
                                  const PmidName& pmid_name,
                                  const maidsafe_error& result);

  void SendSync(const NodeId& destination_peer,
                const MaidName& account_name,
                const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer,
                           const MaidName& account_name,
                           const std::string& serialised_account);

 private:
  MaidManagerDispatcher();
  MaidManagerDispatcher(const MaidManagerDispatcher&);
  MaidManagerDispatcher(MaidManagerDispatcher&&);
  MaidManagerDispatcher& operator=(MaidManagerDispatcher);

  routing::GroupSource Sender(const MaidName& account_name) const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  static const nfs::Persona kSourcePersona_;
};



// ==================== Implementation =============================================================
template<typename Data>
void MaidManagerDispatcher::SendGetVersionRequest(const MaidName& account_name,
                                                  const typename Data::name_type& data_name) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kGetRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kVersionManager);
  static const DataTagValue kDataEnumValue(Data::name_type::tag_type::kEnumValue);

  nfs::Message::Data inner_data(kDataEnumValue, data_name.data, NonEmptyString(), kAction);
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::GroupId(NodeId(data_name->string())), cacheable);
  routing_.Send(message);
}

template<typename Data>
void MaidManagerDispatcher::SendPutRequest(const MaidName& account_name,
                                           const Data& data,
                                           const PmidName& pmid_node_hint) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(is_cacheable<Data>::value ? routing::Cacheable::kGet :
                                                                        routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kDataManager);
  static const DataTagValue kDataEnumValue(Data::name_type::tag_type::kEnumValue);

  nfs::Message::Data inner_data(kDataEnumValue, data.name().data, data.Serialise().data, kAction);
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data, pmid_node_hint);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::GroupId(NodeId(data.name()->string())), cacheable);
  routing_.Send(message);
}

template<>
void MaidManagerDispatcher::SendPutRequest<OwnerDirectory>(const MaidName& /*account_name*/,
                                                           const OwnerDirectory& /*data*/,
                                                           const PmidName& /*pmid_node_hint*/) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(is_cacheable<OwnerDirectory>::value ?
                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kOwnerDirectoryManager);
  static const DataTagValue kDataEnumValue(OwnerDirectory::name_type::tag_type::kEnumValue);
  // TODO(Fraser#5#): 2013-08-03 - Handle
}

template<>
void MaidManagerDispatcher::SendPutRequest<GroupDirectory>(const MaidName& /*account_name*/,
                                                           const GroupDirectory& /*data*/,
                                                           const PmidName& /*pmid_node_hint*/) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(is_cacheable<GroupDirectory>::value ?
                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kGroupDirectoryManager);
  static const DataTagValue kDataEnumValue(GroupDirectory::name_type::tag_type::kEnumValue);
  // TODO(Fraser#5#): 2013-08-03 - Handle
}

template<>
void MaidManagerDispatcher::SendPutRequest<WorldDirectory>(const MaidName& /*account_name*/,
                                                           const WorldDirectory& /*data*/,
                                                           const PmidName& /*pmid_node_hint*/) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(is_cacheable<WorldDirectory>::value ?
                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kWorldDirectoryManager);
  static const DataTagValue kDataEnumValue(WorldDirectory::name_type::tag_type::kEnumValue);
  // TODO(Fraser#5#): 2013-08-03 - Handle
}

template<typename Data>
void MaidManagerDispatcher::SendPutResponse(const MaidName& account_name,
                                            const typename Data::name_type& data_name,
                                            const maidsafe_error& result) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kPutResponse);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);
  static const DataTagValue kDataEnumValue(Data::name_type::tag_type::kEnumValue);

  nfs::Message::Data inner_data(result);
  inner_data.type = kDataEnumValue;
  inner_data.name = data_name.data;
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(NodeId(account_name->string())), cacheable);
  routing_.Send(message);
}

template<typename Data>
void MaidManagerDispatcher::SendDeleteRequest(const MaidName& account_name,
                                              const typename Data::name_type& data_name) {
  typedef routing::GroupToGroupMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kDeleteRequest);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kDataManager);
  static const DataTagValue kDataEnumValue(Data::name_type::tag_type::kEnumValue);

  nfs::Message::Data inner_data(kDataEnumValue, data_name.data, NonEmptyString(), kAction);
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::GroupId(NodeId(data_name->string())), cacheable);
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_
