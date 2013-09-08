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
  void SendPutRequest(const nfs::MessageId& task_id,
                      const Data& data,
                      const PmidName& pmid_node);
  template<typename Data>
  void SendDeleteRequest(const nfs::MessageId& task_id,
                         const PmidName& pmid_node,
                         const typename Data::Name& data_name);
  template<typename Data>
  void SendPutResponse(const nfs::MessageId& task_id,
                       const PmidName& pmid_node,
                       const Data::Name& data_name,
                       const maidsafe_error& error_code);
  void SendStateChange(const PmidName& pmid_node, const Data::Name& data_name);
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
void PmidManagerDispatcher::SendPutRequest(const nfs::MessageId& task_id,
                                           const Data& data,
                                           const PmidName& pmid_node) {
  typedef nfs::PutRequestFromPmidManagerToPmidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  nfs_vault::DataNameAndContent data_name_and_content;
  data_name_and_content.name = nfs_vault::DataName(data.name());
  data_name_and_content.content = data.content();
  NfsMessage nfs_message(task_id, data_name_and_content);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(
                             routing::GroupSource(routing::GroupId(pmid_node),
                                                  routing::SingleId(routing_.kNodeId()))),
                         NfsMessage::Receiver(routing::SingleId(NodeId(data.name()->string()))));
  routing_.Send(message);
}


template<typename Data>
void PmidManagerDispatcher::SendDeleteRequest(const nfs::MessageId& task_id,
                                              const PmidName& pmid_node,
                                              const typename Data::Name& data_name) {
  typedef nfs::DeleteRequestFromPmidManagerToPmidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  nfs_vault::DataName data;
  data = nfs_vault::DataName(data_name);
  NfsMessage nfs_message(task_id, data);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(
                             routing::GroupSource(routing::GroupId(pmid_node),
                                                  routing::SingleId(routing_.kNodeId()))),
                         NfsMessage::Receiver(routing::SingleId(NodeId(data_name->string()))));
  routing_.Send(message);
}

template<typename Data>
void PmidManagerDispatcher::SendPutResponse(const nfs::MessageId& task_id,
                                            const PmidName& pmid_node,
                                            const Data::Name& data_name,
                                            const maidsafe_error& error_code) {
  typedef nfs::PutResponseFromPmidManagerToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  nfs::DataNameAndReturnCode data;
  data.name = nfs::DataName(data_name);
  data.return_code = nfs::ReturnCode(error_code);
  NfsMessage nfs_message(task_id, data);
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(pmid_node),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
