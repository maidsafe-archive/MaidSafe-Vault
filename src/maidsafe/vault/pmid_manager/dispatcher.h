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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_

#include <string>
#include <vector>

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_type_values.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/client/messages.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class PmidManagerDispatcher {
 public:
  explicit PmidManagerDispatcher(routing::Routing& routing);

  template <typename Data>
  void SendPutRequest(const Data& data, const PmidName& pmid_node,
                      nfs::MessageId message_id);
  template <typename Data>
  void SendDeleteRequest(const PmidName& pmid_node, const typename Data::Name& data_name,
                         nfs::MessageId message_id);
  template <typename Data>
  void SendPutResponse(const typename Data::Name& data_name, const PmidName& pmid_node,
                       nfs::MessageId message_id);

  template <typename Data>
  void SendPutFailure(const typename Data::Name& name, uint64_t size, const PmidName& pmid_node,
                      const maidsafe_error& error_code, nfs::MessageId message_id);

  //  void SendStateChange(const PmidName& pmid_node, const typename Data::Name& data_name);
  template <typename KeyType>
  void SendSync(const KeyType& key, const std::string& serialised_sync);
  void SendAccountTransfer(const NodeId& destination_peer, const std::string& serialised_account);
  void SendAccountQuery(const PmidManager::Key& key);
  void SendAccountQueryResponse(const std::string& serialised_account,
                                const routing::GroupId& group_id, const NodeId& sender);

 private:
  PmidManagerDispatcher();
  PmidManagerDispatcher(const PmidManagerDispatcher&);
  PmidManagerDispatcher(PmidManagerDispatcher&&);
  PmidManagerDispatcher& operator=(PmidManagerDispatcher);

  template <typename Message>
  void CheckSourcePersonaType() const;
  routing::GroupSource Sender(const MaidName& account_name) const;

  routing::Routing& routing_;
};

// ==================== Implementation =============================================================

template <typename Data>
void PmidManagerDispatcher::SendPutRequest(const Data& data, const PmidName& pmid_node,
                                           nfs::MessageId message_id) {
  LOG(kVerbose) << "PmidManagerDispatcher SendPutRequest to pmid_node -- "
                << HexSubstr(pmid_node.value.string())
                << " , with message_id -- " << message_id.data;
  typedef PutRequestFromPmidManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, nfs_vault::DataNameAndContent(data));
  RoutingMessage message(
      vault_message.Serialise(),
      VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                           routing::SingleId(routing_.kNodeId())),
      VaultMessage::Receiver(routing::SingleId(NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

template <typename Data>
void PmidManagerDispatcher::SendDeleteRequest(const PmidName& pmid_node,
                                              const typename Data::Name& data_name,
                                              nfs::MessageId message_id) {
  typedef DeleteRequestFromPmidManagerToPmidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(message_id, nfs_vault::DataName(data_name));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_node.value.string())));
  routing_.Send(message);
}

template<typename Data>
void PmidManagerDispatcher::SendPutResponse(const typename Data::Name& data_name,
                                            const PmidName& pmid_node,
                                            nfs::MessageId message_id) {
  typedef PutResponseFromPmidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id, nfs_vault::DataName(data_name));
  CheckSourcePersonaType<VaultMessage>();
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(data_name.value)));
  routing_.Send(message);
}

template <typename Data>
void PmidManagerDispatcher::SendPutFailure(const typename Data::Name& name, uint64_t size,
                                           const PmidName& pmid_node,
                                           const maidsafe_error& error_code,
                                           nfs::MessageId message_id) {
  typedef PutFailureFromPmidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage vault_message(
      message_id,
      nfs_client::DataNameAndSizeAndReturnCode(nfs_vault::DataName(name), size,
                                               nfs_client::ReturnCode(error_code)));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(pmid_node.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(name.value.string())));
  routing_.Send(message);
}

template <typename KeyType>
void PmidManagerDispatcher::SendSync(const KeyType& key, const std::string& serialised_sync) {
  typedef SynchroniseFromPmidManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  SendSyncMessage<VaultMessage> sync_sender;
  sync_sender(routing_, VaultMessage((nfs_vault::Content(serialised_sync))),
              PmidName(key.group_name()));
}

// ==================== General implementation =====================================================

template<typename Message>
void PmidManagerDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == nfs::Persona::kPmidManager,
                "The source Persona must be kPmidManager.");
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_DISPATCHER_H_
