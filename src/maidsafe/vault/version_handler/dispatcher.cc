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


#include "maidsafe/vault/version_handler/dispatcher.h"

#include <string>

#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/message_types.h"

namespace maidsafe {

namespace vault {

VersionHandlerDispatcher::VersionHandlerDispatcher(routing::Routing& routing)
    : routing_(routing) {}

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const VersionHandler::Key& key,
    const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& return_code,
    nfs::MessageId message_id) {
  typedef nfs::GetVersionsResponseFromVersionHandlerToDataGetter NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message;
  nfs_message.id = message_id;
  nfs_message.contents.reset(new nfs_client::StructuredDataNameAndContentOrReturnCode());
  if (return_code.code() == CommonErrors::success) {
    nfs_message.contents->structured_data.reset(nfs_client::StructuredData(versions));
  } else {
    nfs_message.contents->data_name = nfs_vault::DataName(key.type, key.name);
    nfs_message.contents->return_code = nfs_client::ReturnCode(return_code);
  }
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(requestor.node_id));
  routing_.Send(message);
}

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& return_code, nfs::MessageId message_id) {
  typedef nfs::GetVersionsResponseFromVersionHandlerToMaidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message;
  nfs_message.id = message_id;
  nfs_message.contents.reset(new nfs_client::StructuredDataNameAndContentOrReturnCode());
  if (return_code.code() == CommonErrors::success) {
    nfs_message.contents->structured_data.reset(nfs_client::StructuredData(versions));
  } else {
    nfs_message.contents->data_name = nfs_vault::DataName(key.type, key.name);
    nfs_message.contents->return_code = nfs_client::ReturnCode(return_code);
  }
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(requestor.node_id));
  routing_.Send(message);
}

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& return_code, nfs::MessageId message_id) {
  typedef nfs::GetBranchResponseFromVersionHandlerToDataGetter NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message;
  nfs_message.id = message_id;
  nfs_message.contents.reset(new nfs_client::StructuredDataNameAndContentOrReturnCode());
  if (return_code.code() == CommonErrors::success) {
    nfs_message.contents->structured_data = nfs_client::StructuredData(versions);
  } else {
    nfs_message.contents->data_name = nfs_vault::DataName(key.type, key.name);
    nfs_message.contents->return_code = nfs_client::ReturnCode(return_code);
  }
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(requestor.node_id));
  routing_.Send(message);
}

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& return_code, nfs::MessageId message_id) {
  typedef nfs::GetBranchResponseFromVersionHandlerToMaidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message;
  nfs_message.id = message_id;
  nfs_message.contents.reset(new nfs_client::StructuredDataNameAndContentOrReturnCode());
  if (return_code.code() == CommonErrors::success) {
    nfs_message.contents->structured_data = nfs_client::StructuredData(versions);
  } else {
    nfs_message.contents->data_name = nfs_vault::DataName(key.type, key.name);
    nfs_message.contents->return_code = nfs_client::ReturnCode(return_code);
  }
  RoutingMessage message(nfs_message.Serialise(),
                         NfsMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         NfsMessage::Receiver(requestor.node_id));
  routing_.Send(message);
}

void VersionHandlerDispatcher::SendCreateVersionTreeResponse(
    const Identity& originator, const VersionHandler::Key& key, const maidsafe_error& return_code,
        nfs::MessageId message_id) {
  typedef CreateVersionTreeResponseFromVersionHandlerToMaidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message;
  vault_message.id = message_id;
  vault_message.contents.reset(new nfs_client::ReturnCode(return_code));

  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(originator.string())));
  routing_.Send(message);
}

void VersionHandlerDispatcher::SendPutVersionResponse(
    const Identity& originator,
    const VersionHandler::Key& key,
    const VersionHandler::VersionName& tip_of_tree,
    const maidsafe_error& return_code,
    nfs::MessageId message_id) {
  typedef PutVersionResponseFromVersionHandlerToMaidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message;
  vault_message.id = message_id;
  vault_message.contents.reset(new nfs_client::TipOfTreeAndReturnCode());
  vault_message.contents->return_code = nfs_client::ReturnCode(return_code);
  if (tip_of_tree != VersionHandler::VersionName())
    vault_message.contents->tip_of_tree = tip_of_tree;
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(NodeId(key.name.string())),
                                            routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(NodeId(originator.string())));
  routing_.Send(message);
}

// ==================== Sync / AccountTransfer implementation ======================================
void VersionHandlerDispatcher::SendSync(const VersionHandler::Key& key,
                                        const std::string& serialised_sync) {
  typedef SynchroniseFromVersionHandlerToVersionHandler VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  SendSyncMessage<VaultMessage> sync_sender;
  sync_sender(routing_, VaultMessage((nfs_vault::Content(serialised_sync))), key);
}

void VersionHandlerDispatcher::SendAccountTransfer(const NodeId& destination_peer,
                                                   nfs::MessageId message_id,
                                                   const std::string& serialised_account) {
  typedef AccountTransferFromVersionHandlerToVersionHandler VaultMessage;
  CheckSourcePersonaType<VaultMessage>();
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  VaultMessage vault_message(message_id, nfs_vault::Content(serialised_account));
  RoutingMessage message(vault_message.Serialise(),
                         VaultMessage::Sender(routing::GroupId(destination_peer),
                                              routing::SingleId(routing_.kNodeId())),
                         VaultMessage::Receiver(routing::SingleId(destination_peer)));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
