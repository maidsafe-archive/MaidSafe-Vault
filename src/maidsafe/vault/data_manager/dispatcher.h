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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_

#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class DataManagerDispatcher {
 public:
  DataManagerDispatcher(routing::Routing& routing, const passport::Pmid& signing_fob)
      : routing_(routing), kSigningFob_(signing_fob) {}

  // To PmidManager
  template <typename Data>
  void SendGetRequest(const PmidName& pmid_node, const typename Data::Name& data_name,
                      const nfs::MessageId& message_id);
  // To MaidNode
  template <typename Data>
  void SendGetResponse(const MaidName& maid_node, const Data& data,
                       const nfs::MessageId& message_id);
  // To MaidNode (failure)
  template <typename Data>
  void SendGetResponse(const MaidName& maid_node, const typename Data::Name& data_name,
                       const maidsafe_error& result, const nfs::MessageId& message_id);

  // To PmidNode
  template <typename Data>
  void SendGetResponse(const PmidName& pmid_node, const Data& data,
                       const nfs::MessageId& message_id);

  // To PmidManager
  template <typename Data>
  void SendPutRequest(const PmidName& pmid_name, const Data& data,
                      const nfs::MessageId& message_id);

  // To MaidManager
  template <typename Data>
  void SendPutResponse(const MaidName& account_name, const typename Data::Name& data_name,
                       const int32_t& cost, const nfs::MessageId& message_id);

  // To PmidManager
  template <typename Data>
  void SendDeleteRequest(const PmidName& pmid_name, const typename Data::Name& data_name,
                         const nfs::MessageId& message_id);

  // To MaidManager
  template <typename Data>
  void SendPutFailure(const MaidName& maid_node, const typename Data::Name& data_name,
                      const maidsafe_error& error, const nfs::MessageId& message_id);

  template <typename Data>
  void SendIntegrityCheck(const typename Data::Name& name, const NonEmptyString& random_string,
                          const PmidName& pmid_node, const nfs::MessageId& message_id);

  void SendSync(const Identity& data_name, const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer, const MaidName& account_name,
                           const std::string& serialised_account);

 private:
  DataManagerDispatcher();
  DataManagerDispatcher(const DataManagerDispatcher&);
  DataManagerDispatcher(DataManagerDispatcher&&);
  DataManagerDispatcher& operator=(DataManagerDispatcher);

  template <typename Data>
  routing::GroupSource Sender(const typename Data::Name& data_name) const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  const routing::SingleId kThisNodeAsSender_;
};

// ==================== Implementation =============================================================

// To PmidManager
template <typename Data>
void DataManagerDispatcher::SendGetRequest(const PmidName& /*pmid_node*/,
                                           const typename Data::Name& /*data_name*/,
                                           const nfs::MessageId& /*message_id*/) {
  //  typedef nfs::GetRequestFromDataManagerToPmidNode NfsMessage;
  //  CheckSourcePersonaType<NfsMessage>();
  //  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  //  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ?
  // routing::Cacheable::kGet :
  //                                                                         routing::Cacheable::kNone);
  //  NfsMessage nfs_message((message_id, NfsMessage::Contents(data_name)));
  //  NfsMessage::Receiver receiver(routing::SingleId(NodeId(pmid_node->string())));
  //  routing_.Send(RoutingMessage(nfs_message.Serialise(), Sender(data.name), receiver,
  // kCacheable));
}

// To MaidNode
// template<typename Data>
// void SendGetResponse(const MaidName& maid_node,
//                     const Data& data,
//                     nfs::MessageId message_id) {
//  typedef SendGetResponseFromDataManagerToMaidNode NfsMessage;
//  CheckSourcePersonaType<NfsMessage>();
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
//  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kPut
// :
//                                                                         routing::Cacheable::kNone);
//  NfsMessage::Contents contents;
//  *contents.data = data;
//  NfsMessage nfs_message((message_id, contents));
//  NfsMessage::Receiver receiver(routing::SingleId(NodeId(maid_node->string())));
//  routing_.Send(RoutingMessage(nfs_message.Serialise(), Sender(data.name), receiver, kCacheable));
//}

template <typename Data>
void DataManagerDispatcher::SendPutFailure(
    const MaidName& maid_node, const typename Data::Name& data_name, const maidsafe_error& error,
    const nfs::MessageId& message_id) {
  typedef PutFailureFromDataManagerToMaidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id,
                             nfs_client::DataNameAndReturnCode(data_name,
                                                               nfs_client::ReturnCode(error)));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(data_name.value)),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::GroupId(
                                                    NodeId(maid_node.value.string()))));
  routing_.Send(message);
}


template <typename Data>
void DataManagerDispatcher::SendPutRequest(const PmidName& pmid_name, const Data& data,
                                           const nfs::MessageId& message_id) {
  typedef PutRequestFromDataManagerToPmidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndContent(data));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(data.name().string()),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_name.value.string())));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendPutResponse(const MaidName& account_name,
                                            const typename Data::Name& data_name,
                                            const int32_t& cost, const nfs::MessageId& message_id) {
  typedef PutResponseFromDataManagerToMaidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndCost(data_name, cost));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(data_name().string()),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(account_name.value)));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendIntegrityCheck(const typename Data::Name& name,
                                               const NonEmptyString& random_string,
                                               const PmidName& pmid_node,
                                               const nfs::MessageId& message_id) {
  typedef IntegrityCheckRequestFromDataManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(
      message_id, nfs_vault::DataNameAndRandomString(name, random_string));
  RoutingMessage message(
      vault_message.Serialise(),
      VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
      VaultMessage::Receiver(routing::SingleId(NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendDeleteRequest(const PmidName& pmid_node,
                                              const typename Data::Name& name,
                                              const nfs::MessageId& message_id) {
  typedef DeleteRequestFromDataManagerToPmidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id, nfs_vault::DataName(name));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_node.value.string())));
  routing_.Send(message);
}


// template<typename Data>
// routing::GroupSource MaidManagerDispatcher::Sender(const typename Data::Name& data_name) const {
//  return routing::GroupSource(routing::GroupId(NodeId(data_name->string())),
//                              routing::SingleId(routing_.kNodeId()));
//}

// template<typename Message>
// void MaidNodeDispatcher::CheckSourcePersonaType() const {
//  static_assert(NfsMessage::SourcePersona::value == Persona::kDataManager,
//                  "The source Persona must be kDataManager.");
//}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_
