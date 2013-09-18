/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class PmidManagerDispatcher {
 public:
  PmidManagerDispatcher(routing::Routing& routing);

  template<typename Data>
  void SendPutRequest(const Data& data,
                      const PmidName& pmid_node,
                      const nfs::MessageId& message_id);
  template<typename Data>
  void SendDeleteRequest(const nfs::MessageId& message_id,
                         const PmidName& pmid_node,
                         const typename Data::Name& data_name);
  template<typename Data>
  void SendPutResponse(const Data& data,
                       const PmidName& pmid_node,
                       const nfs::MessageId& message_id,
                       const maidsafe_error& error_code);

  // Handing suceess
  template<typename Data>
  void SendPutResponse(const typename Data::Name& name,
                       const PmidName& pmid_node,
                       const nfs::MessageId& message_id,
                       const maidsafe_error& error_code);

//  void SendStateChange(const PmidName& pmid_node, const typename Data::Name& data_name);
  void SendSync(const PmidName& pmid_node, const std::string& serialised_sync);
  void SendAccountTransfer(const PmidName& destination_peer,
                           const PmidName& pmid_node,
                           const std::string& serialised_account);
  void SendPmidAccount(const PmidName& pmid_node, const std::string& serialised_account_response);

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
void PmidManagerDispatcher::SendPutRequest(const Data& data,
                                           const PmidName& pmid_node,
                                           const nfs::MessageId& message_id) {
  typedef nfs::PutRequestFromPmidManagerToPmidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message(message_id,
                         nfs_vault::DataNameAndContent(Data::Name::data_type,
                                                       data.name(),
                                                       data.data()));
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(routing::SingleId(NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

template<typename Data>
void PmidManagerDispatcher::SendDeleteRequest(const nfs::MessageId& message_id,
                                              const PmidName& pmid_node,
                                              const typename Data::Name& data_name) {
  typedef nfs::DeleteRequestFromPmidManagerToPmidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message(message_id, nfs_vault::DataName(data_name.type, data_name.raw_name));
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(data_name().string())));
  routing_.Send(message);
}

template<typename Data>
void PmidManagerDispatcher::SendPutResponse(const Data& data,
                                            const PmidName& pmid_node,
                                            const nfs::MessageId& message_id,
                                            const maidsafe_error& error_code) {
  typedef nfs::PutResponseFromPmidManagerToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message(message_id,
                         nfs_client::DataNameAndContentAndReturnCode(
                             data.name.type,
                             data.name(),
                             data.data(),
                             nfs_client::ReturnCode(error_code)));
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(data.name()->string())));
  routing_.Send(message);
}

template<typename Data>
void PmidManagerDispatcher::SendPutResponse(const typename Data::Name& name,
                                            const PmidName& pmid_node,
                                            const nfs::MessageId& message_id,
                                            const maidsafe_error& error_code) {
  typedef nfs::PutResponseFromPmidManagerToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message(message_id,
                         nfs_client::DataNameAndContentAndReturnCode(
                             name.type,
                             name.raw_name,
                             nfs_client::ReturnCode(error_code)));
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(name.raw_name.string())));
  routing_.Send(message);
}



}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
