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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_

#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_type_values.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

class OwnerDirectory;
class GroupDirectory;
class WorldDirectory;

namespace vault {

class MaidManagerDispatcher {
 public:
  MaidManagerDispatcher(routing::Routing& routing, const passport::Pmid& signing_fob);

  template <typename Data>
  void SendGetVersionRequest(const MaidName& account_name, const typename Data::Name& data_name);

  template <typename Data>
  void SendPutRequest(const MaidName& account_name, const Data& data,
                      const PmidName& pmid_node_hint, nfs::MessageId message_id);

  template <typename Data>
  void SendPutResponse(const MaidName& account_name, const typename Data::Name& data_name,
                       const maidsafe_error& result, nfs::MessageId message_id);

  void SendDeleteRequest(const MaidName& account_name, const nfs_vault::DataName& data_name,
                         nfs::MessageId message_id);

  template <typename DataNameType>
  void SendPutVersionRequest(const MaidName& maid_name, const DataNameType& data_name,
                             const StructuredDataVersions::VersionName&  old_name,
                             const StructuredDataVersions::VersionName&  new_name,
                             nfs::MessageId message_id);

  void SendPutVersionResponse(const MaidName& maid_name, const maidsafe_error& return_code,
                              std::unique_ptr<StructuredDataVersions::VersionName> tip_of_tree,
                              nfs::MessageId message_id);

  template <typename DataNameType>
  void SendDeleteBranchUntilFork(const MaidName& maid_name, const DataNameType& data_name,
                                 const StructuredDataVersions::VersionName& version,
                                 nfs::MessageId message_id);

  void SendCreateAccountResponse(const MaidName& account_name, const maidsafe_error& result,
                                 nfs::MessageId message_id);

  void SendRemoveAccountResponse(const MaidName& account_name, const maidsafe_error& result,
                                 nfs::MessageId message_id);

  void SendUnregisterPmidResponse(const MaidName& account_name, const PmidName& pmid_name,
                                  const maidsafe_error& result, nfs::MessageId message_id);
  template <typename KeyType>
  void SendSync(const KeyType& key, const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer, const MaidName& account_name,
                           const std::string& serialised_account);

  template <typename Data>
  void SendPutFailure(const MaidName& maid_node, const typename Data::Name& data_name,
                      const maidsafe_error& error,  nfs::MessageId message_id);

  void SendPmidHealthRequest(const MaidName& maid_name, const PmidName& pmid_node,
                             nfs::MessageId message_id);

  void SendPmidHealthResponse(const MaidName& maid_name, int64_t available_size,
                              const maidsafe_error& return_code, nfs::MessageId message_id);

  void SendCreatePmidAccountRequest(const passport::PublicMaid& account_name,
                                    const passport::PublicPmid& pmid_name);

  template <typename DataNameType>
  void SendCreateVersionTreeRequest(const MaidName& maid_name, const DataNameType& data_name,
      const StructuredDataVersions::VersionName& version, uint32_t max_versions,
      uint32_t max_branches, nfs::MessageId message_id);

  void SendCreateVersionTreeResponse(const MaidName& maid_name, const maidsafe_error& error,
                                     nfs::MessageId message_id);

  void SendRegisterPmidResponse(const MaidName& maid_name, const maidsafe_error& error,
                                nfs::MessageId message_id);

 private:
  MaidManagerDispatcher();
  MaidManagerDispatcher(const MaidManagerDispatcher&);
  MaidManagerDispatcher(MaidManagerDispatcher&&);
  MaidManagerDispatcher& operator=(MaidManagerDispatcher);

  typedef detail::GroupOrKeyType<MaidManager> GroupOrKeyHelper;

  template <typename Message>
  void CheckSourcePersonaType() const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  static const nfs::Persona kSourcePersona_ = nfs::Persona::kMaidManager;
};

// ==================== Implementation =============================================================

template <typename Data>
void MaidManagerDispatcher::SendPutRequest(const MaidName& account_name, const Data& data,
                                           const PmidName& pmid_node_hint,
                                           nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerDispatcher SendPutRequest to pmid_node_hint -- "
                << HexSubstr(pmid_node_hint.value.string())
                << " , with message_id -- " << message_id.data
                << " of account " << HexSubstr(account_name.value.string());
  typedef PutRequestFromMaidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();

  VaultMessage vault_message(
      message_id,
      nfs_vault::DataAndPmidHint(nfs_vault::DataName(data.name()), data.Serialise(),
                                 pmid_node_hint));
  RoutingMessage message(vault_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, account_name),
                         VaultMessage::Receiver(routing::GroupId(NodeId(data.name()))));
  routing_.Send(message);
}

template <typename Data>
void MaidManagerDispatcher::SendPutFailure(
    const MaidName& maid_name, const typename Data::Name& data_name, const maidsafe_error& error,
    nfs::MessageId message_id) {
  typedef nfs::PutFailureFromMaidManagerToMaidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<NfsMessage>();

  NfsMessage nfs_message(message_id,
                         nfs_client::DataNameAndReturnCode(data_name,
                                                           nfs_client::ReturnCode(error)));
  RoutingMessage message(nfs_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, maid_name),
                         NfsMessage::Receiver(routing::SingleId(NodeId(data_name.value))));
  routing_.Send(message);
}

template <typename DataNameType>
void MaidManagerDispatcher::SendPutVersionRequest(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& old_version,
    const StructuredDataVersions::VersionName& new_version, nfs::MessageId message_id) {
  typedef PutVersionRequestFromMaidManagerToVersionHandler VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage valut_message(message_id, nfs_vault::DataNameOldNewVersion(data_name, old_version,
                                                                          new_version));
  RoutingMessage message(valut_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, maid_name),
                         VaultMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}

template <typename DataNameType>
void MaidManagerDispatcher::SendDeleteBranchUntilFork(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& version, nfs::MessageId message_id) {
  typedef DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage valut_message(message_id, nfs_vault::DataNameAndVersion(data_name, version));
  RoutingMessage message(valut_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, maid_name),
                         VaultMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}

template <typename DataNameType>
void MaidManagerDispatcher::SendCreateVersionTreeRequest(const MaidName& maid_name,
    const DataNameType& data_name, const StructuredDataVersions::VersionName& version,
    uint32_t max_versions, uint32_t max_branches, nfs::MessageId message_id) {
  typedef CreateVersionTreeRequestFromMaidManagerToVersionHandler VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  VaultMessage valut_message(message_id,
                             nfs_vault::VersionTreeCreation(data_name, version, max_versions,
                                                             max_branches));
  RoutingMessage message(valut_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, maid_name),
                         VaultMessage::Receiver(NodeId(data_name->string())));
  routing_.Send(message);
}

// template<>
// void MaidManagerDispatcher::SendPutRequest<OwnerDirectory>(const MaidName& /*account_name*/,
//                                                           const OwnerDirectory& /*data*/,
//                                                           const PmidName& /*pmid_node_hint*/,
//                                                           nfs::MessageId /*message_id*/) {
//  typedef routing::GroupToGroupMessage RoutingMessage;
//  static const routing::Cacheable cacheable(is_cacheable<OwnerDirectory>::value ?
//                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
//  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
//  static const nfs::Persona kDestinationPersona(nfs::Persona::kVersionHandler);
//  static const DataTagValue kDataEnumValue(OwnerDirectory::Tag::kValue);
// TODO(Fraser#5#): 2013-08-03 - Handle
// }

// template<>
// void MaidManagerDispatcher::SendPutRequest<GroupDirectory>(const MaidName& /*account_name*/,
//                                                           const GroupDirectory& /*data*/,
//                                                           const PmidName& /*pmid_node_hint*/,
//                                                           nfs::MessageId /*message_id*/) {
//  typedef routing::GroupToGroupMessage RoutingMessage;
//  static const routing::Cacheable cacheable(is_cacheable<GroupDirectory>::value ?
//                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
//  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
//  static const nfs::Persona kDestinationPersona(nfs::Persona::kVersionHandler);
//  static const DataTagValue kDataEnumValue(GroupDirectory::Tag::kValue);
// TODO(Fraser#5#): 2013-08-03 - Handle
// }

// template<>
// void MaidManagerDispatcher::SendPutRequest<WorldDirectory>(const MaidName& /*account_name*/,
//                                                           const WorldDirectory& /*data*/,
//                                                           const PmidName& /*pmid_node_hint*/,
//                                                           nfs::MessageId /*message_id*/) {
//  typedef routing::GroupToGroupMessage RoutingMessage;
//  static const routing::Cacheable cacheable(is_cacheable<WorldDirectory>::value ?
//                                            routing::Cacheable::kGet : routing::Cacheable::kNone);
//  static const nfs::MessageAction kAction(nfs::MessageAction::kPutRequest);
//  static const nfs::Persona kDestinationPersona(nfs::Persona::kVersionHandler);
//  static const DataTagValue kDataEnumValue(WorldDirectory::Tag::kValue);
// TODO(Fraser#5#): 2013-08-03 - Handle
// }

// template<typename Data>
// void MaidManagerDispatcher::SendPutResponse(const MaidName& account_name,
//                                            const typename Data::Name& data_name,
//                                            const maidsafe_error& result,
//                                            nfs::MessageId /*message_id*/) {
//  typedef routing::GroupToSingleMessage RoutingMessage;
//  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
//  static const nfs::MessageAction kAction(nfs::MessageAction::kPutResponse);
//  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);
//  static const DataTagValue kDataEnumValue(Data::Tag::kValue);

//  nfs::Message::Data inner_data(result);
//  inner_data.type = kDataEnumValue;
//  inner_data.name = data_name.data;
//  inner_data.action = kAction;
//  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
//  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
//                         routing::SingleId(NodeId(account_name->string())), cacheable);
//  routing_.Send(message);
// }


template <typename KeyType>
void MaidManagerDispatcher::SendSync(const KeyType& key, const std::string& serialised_sync) {
  typedef SynchroniseFromMaidManagerToMaidManager VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  SendSyncMessage<VaultMessage> sync_sender;
  sync_sender(routing_, VaultMessage((nfs_vault::Content(serialised_sync))), key.group_name());
}

// ==================== General implementation =====================================================

template<typename Message>
void MaidManagerDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == nfs::Persona::kMaidManager,
                "The source Persona must be kMaidManager.");
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_DISPATCHER_H_
