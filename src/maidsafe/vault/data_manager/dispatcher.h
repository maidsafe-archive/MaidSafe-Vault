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
#include "maidsafe/common/data_types/data_type_values.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/message_types_partial.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/data_manager/data_manager.h"

namespace maidsafe {

namespace vault {

class DataManagerDispatcher {
 public:
  DataManagerDispatcher(routing::Routing& routing, const passport::Pmid& signing_fob)
      : routing_(routing), kSigningFob_(signing_fob) {}

  // =========================== Put section =======================================================
  // To PmidManager
  template <typename Data>
  void SendPutRequest(const PmidName& pmid_name, const Data& data, nfs::MessageId message_id);

  // To MaidManager
  template <typename Data>
  void SendPutResponse(const MaidName& account_name, const typename Data::Name& data_name,
                       uint64_t cost, nfs::MessageId message_id);

  // To MaidManager
  template <typename Data>
  void SendPutFailure(const MaidName& maid_node, const typename Data::Name& data_name,
                      const maidsafe_error& error, nfs::MessageId message_id);

  template <typename Data>
  void SendPmidUpdateAccount(const typename Data::Name& data_name, const PmidName& pmid_node,
                             uint64_t chunk_size, uint64_t given_size);
  // =========================== Get section (includes integrity checks) ===========================
  // To PmidNode
  template <typename Data>
  void SendGetRequest(const PmidName& pmid_node, const typename Data::Name& data_name,
                      nfs::MessageId message_id);

  // To PmidNode
  template <typename Data>
  void SendIntegrityCheck(const typename Data::Name& data_name, const NonEmptyString& random_string,
                          const PmidName& pmid_node, nfs::MessageId message_id);

  // To MaidNode or DataGetter
  template <typename RequestorIdType, typename Data>
  void SendGetResponseSuccess(const RequestorIdType& requestor_id, const Data& data,
                              nfs::MessageId message_id);

  // To MaidNode or DataGetter
  template <typename RequestorIdType, typename DataName>
  void SendGetResponseFailure(const RequestorIdType& requestor_id, const DataName& data_name,
                              const maidsafe_error& result, nfs::MessageId message_id);

  // =========================== Delete section ====================================================
  // To PmidManager
  template <typename Data>
  void SendDeleteRequest(const PmidName& pmid_name, const typename Data::Name& data_name,
                         const uint64_t size, nfs::MessageId message_id);
  template <typename Data>
  void SendFalseDataNotification(const PmidName& pmid_node,
                                 const typename Data::Name& data_name,
                                 const uint64_t size,
                                 nfs::MessageId message_id);

  // =========================== Sync / AccountTransfer section ====================================
  void SendSync(const DataManager::Key& key, const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer,
                           const std::string& serialised_account);
  void SendAccountRequest(const Key& key);
  void SendAccountResponse(const std::string& serialised_account, const routing::GroupId& group_id,
                           const NodeId& sender);

 private:
  DataManagerDispatcher();
  DataManagerDispatcher(const DataManagerDispatcher&);
  DataManagerDispatcher(DataManagerDispatcher&&);
  DataManagerDispatcher& operator=(DataManagerDispatcher);

  template <typename Message>
  void CheckSourcePersonaType() const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  const routing::SingleId kThisNodeAsSender_;
};

// ==================== Put implementation =========================================================
template <typename Data>
void DataManagerDispatcher::SendPutRequest(const PmidName& pmid_name, const Data& data,
                                           nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManager::SendPutRequest to pmid_name "
                << HexSubstr(pmid_name.value.string()) << " with message_id " << message_id.data;
  typedef PutRequestFromDataManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndContent(data));
  RoutingMessage message(vault_message.Serialise(),
                         routing::GroupSource(routing::GroupId(NodeId(data.name().value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_name.value.string())));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendPutResponse(const MaidName& account_name,
                                            const typename Data::Name& data_name,
                                            uint64_t cost, nfs::MessageId message_id) {
  typedef PutResponseFromDataManagerToMaidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndCost(data_name,
                                                                    static_cast<int32_t>(cost)));
  RoutingMessage message(vault_message.Serialise(),
                         routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(account_name.value.string())));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendPutFailure(
    const MaidName& maid_node, const typename Data::Name& data_name, const maidsafe_error& error,
    nfs::MessageId message_id) {
  typedef PutFailureFromDataManagerToMaidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id,
                             nfs_client::DataNameAndReturnCode(data_name,
                                                               nfs_client::ReturnCode(error)));
  RoutingMessage message(
      vault_message.Serialise(),
      routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                           routing::SingleId(routing_.kNodeId())),
      VaultMessage::Receiver(routing::GroupId(NodeId(maid_node.value.string()))));
  routing_.Send(message);
}

// ==================== Get / IntegrityCheck implementation ========================================
template <typename Data>
void DataManagerDispatcher::SendGetRequest(const PmidName& pmid_node,
                                           const typename Data::Name& data_name,
                                           nfs::MessageId message_id) {
  // NB - This should NOT be marked as cacheable - we want to force the message to go all the way to
  // PmidNode to check it's online.
  typedef GetRequestFromDataManagerToPmidNode VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, VaultMessage::Contents(data_name));
  RoutingMessage message(
      vault_message.Serialise(),
      routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                           routing::SingleId(routing_.kNodeId())),
      VaultMessage::Receiver(routing::SingleId(NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendIntegrityCheck(const typename Data::Name& data_name,
                                               const NonEmptyString& random_string,
                                               const PmidName& pmid_node,
                                               nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManagerDispatcher::SendIntegrityCheck send integrity_check request to "
                << HexSubstr(pmid_node->string()) << " for data " << HexSubstr(data_name.value)
                << " with message_id " << message_id.data;
  typedef IntegrityCheckRequestFromDataManagerToPmidNode VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, VaultMessage::Contents(data_name, random_string));
  RoutingMessage message(
      vault_message.Serialise(),
      VaultMessage::Sender(routing::SingleId(routing_.kNodeId())),
      VaultMessage::Receiver(routing::SingleId(NodeId(pmid_node.value.string()))));
  routing_.Send(message);
}

namespace detail {

template <typename RequestorIdType>
struct GetResponseMessage {};

template <>
struct GetResponseMessage<Requestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>> {
  typedef nfs::GetResponseFromDataManagerToMaidNode Type;
};

template <>
struct GetResponseMessage<PartialRequestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>> {
  typedef nfs::GetResponseFromDataManagerToMaidNodePartial Type;
};

template <>
struct GetResponseMessage<Requestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>> {
  typedef nfs::GetResponseFromDataManagerToDataGetter Type;
};

template <>
struct GetResponseMessage<PartialRequestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>> {
  typedef nfs::GetResponseFromDataManagerToDataGetterPartial Type;
};

routing::SingleIdRelay GetDestination(
        const PartialRequestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>& requestor);

routing::SingleIdRelay GetDestination(
        const PartialRequestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>& requestor);

// FIXME after changing requestor in vaults to hold exact sender type
routing::SingleId GetDestination(
        const Requestor<nfs::SourcePersona<nfs::Persona::kMaidNode>>& requestor);

routing::SingleId GetDestination(
        const Requestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>& requestor);

}  // namespace detail

template <typename RequestorIdType, typename Data>
void DataManagerDispatcher::SendGetResponseSuccess(const RequestorIdType& requestor_id,
                                                   const Data& data, nfs::MessageId message_id) {
  typedef typename detail::GetResponseMessage<RequestorIdType>::Type NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kPut :
                                                                         routing::Cacheable::kNone);
  NfsMessage nfs_message(message_id, typename NfsMessage::Contents(data));
  RoutingMessage message(
      nfs_message.Serialise(),
      routing::GroupSource(routing::GroupId(NodeId(data.name().value.string())),
                           routing::SingleId(routing_.kNodeId())),
      typename NfsMessage::Receiver(detail::GetDestination(requestor_id)), kCacheable);
  LOG(kVerbose) << "DataManagerDispatcher::SendGetResponseSuccess routing send msg to "
                << " regarding chunk of "
                << HexSubstr(data.name().value.string());
  routing_.Send(message);
}

template <typename RequestorIdType, typename DataNameType>
void DataManagerDispatcher::SendGetResponseFailure(const RequestorIdType& requestor_id,
                                                   const DataNameType& data_name,
                                                   const maidsafe_error& result,
                                                   nfs::MessageId message_id) {
  typedef typename detail::GetResponseMessage<RequestorIdType>::Type NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<typename NfsMessage::Sender, typename NfsMessage::Receiver>
      RoutingMessage;

  typename NfsMessage::Contents msg_content(data_name, nfs_client::ReturnCode(result));

  NfsMessage nfs_message(message_id, msg_content);
  RoutingMessage message(nfs_message.Serialise(),
                         routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         typename NfsMessage::Receiver(detail::GetDestination(requestor_id)));
  LOG(kVerbose) << "DataManagerDispatcher::SendGetResponseFailure routing send msg to "
//                << HexSubstr(requestor_id.node_id.string())
                << " regarding chunk of "
                << HexSubstr(data_name.value.string());
  routing_.Send(message);
}

// ==================== Delete implementation ======================================================
template <typename Data>
void DataManagerDispatcher::SendDeleteRequest(const PmidName& pmid_node,
                                              const typename Data::Name& data_name,
                                              const uint64_t size,
                                              nfs::MessageId message_id) {
  typedef DeleteRequestFromDataManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndSize(data_name,
                                                                    static_cast<int32_t>(size)));
  RoutingMessage message(vault_message.Serialise(),
                         routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_node.value.string())));
  routing_.Send(message);
}

template <typename Data>
void DataManagerDispatcher::SendFalseDataNotification(const PmidName& pmid_node,
                                                      const typename Data::Name& data_name,
                                                      uint64_t size,
                                                      nfs::MessageId message_id) {
  typedef IntegrityCheckRequestFromDataManagerToPmidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;

  VaultMessage vault_message(message_id, nfs_vault::DataNameAndSize(data_name,
                                                                    static_cast<int32_t>(size)));
  RoutingMessage message(vault_message.Serialise(),
                         routing::GroupSource(routing::GroupId(NodeId(data_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_node.value.string())));
  routing_.Send(message);
}

// ==================== General implementation =====================================================

template<typename Message>
void DataManagerDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == nfs::Persona::kDataManager,
                "The source Persona must be kDataManager.");
}

template <typename Data>
void DataManagerDispatcher::SendPmidUpdateAccount(const typename Data::Name& data_name,
    const PmidName& pmid_node, uint64_t chunk_size, uint64_t given_size) {
  using VaultMessage = UpdateAccountFromDataManagerToPmidManager;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message{ VaultMessage::Contents(static_cast<int32_t>(chunk_size) -
                                                     static_cast<int32_t>(given_size)) };
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(data_name.value.string())),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(pmid_node.value.string())));
  routing_.Send(message);

}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_
