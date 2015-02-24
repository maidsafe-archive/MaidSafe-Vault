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

#ifndef MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_

#include <string>

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/messages.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class PmidNodeDispatcher {
 public:
  explicit PmidNodeDispatcher(routing::Routing& routing);

  void SendGetRequest(const nfs_vault::DataName& data_name);
  void SendGetOrIntegrityCheckResponse(
      const nfs_vault::DataNameAndContentOrCheckResult& data_or_check_result,
      const NodeId& data_manager_node_id,
      nfs::MessageId message_id);
  void SendPmidAccountRequest(const DiskUsage& available_size);

  template <typename Data>
  void SendPutFailure(const typename Data::Name& name,
                      uint64_t size,
                      int64_t available_space,
                      const maidsafe_error& error,
                      nfs::MessageId message_id);
  template <typename Data>
  void SendIntegrityCheckResponse(const typename Data::Name& data_name,
                                  const std::string& hash,
                                  const NodeId& receiver,
                                  const maidsafe_error& error,
                                  nfs::MessageId message_id);

 private:
  PmidNodeDispatcher();
  PmidNodeDispatcher(const PmidNodeDispatcher&);
  PmidNodeDispatcher(PmidNodeDispatcher&&);
  PmidNodeDispatcher& operator=(PmidNodeDispatcher);

  template <typename Message>
  void CheckSourcePersonaType() const;

  routing::Routing& routing_;
};

template <typename Data>
void PmidNodeDispatcher::SendPutFailure(const typename Data::Name& name,
                                        uint64_t size,
                                        int64_t available_space,
                                        const maidsafe_error& error,
                                        nfs::MessageId message_id) {
  typedef PutFailureFromPmidNodeToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(
      message_id, nfs_client::DataNameAndSizeAndSpaceAndReturnCode(name, size, available_space,
                                                                   nfs_client::ReturnCode(error)));
  RoutingMessage routing_message(vault_message.Serialise(),
                                 VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
                                 VaultMessage::Receiver(routing::GroupId(routing_.kNodeId())));
  routing_.Send(routing_message);
}

// ==================== General implementation =====================================================

template<typename Message>
void PmidNodeDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == nfs::Persona::kPmidNode,
                "The source Persona must be kPmidNode.");
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
