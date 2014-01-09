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

#include "maidsafe/vault/maid_manager/dispatcher.h"

#include "maidsafe/nfs/client/messages.h"

namespace maidsafe {

namespace vault {

MaidManagerDispatcher::MaidManagerDispatcher(routing::Routing& routing,
                                             const passport::Pmid& signing_fob)
    : routing_(routing), kSigningFob_(signing_fob) {}

void MaidManagerDispatcher::SendDeleteRequest(const MaidName& account_name,
                                              const nfs_vault::DataName &data_name,
                                              nfs::MessageId message_id) {
  typedef DeleteRequestFromMaidManagerToDataManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();
  LOG(kVerbose) << "MaidManagerDispatcher::SendDeleteRequest for account "
                << HexSubstr(account_name->string()) << " of chunk "
                << HexSubstr(data_name.raw_name.string());
  VaultMessage vault_message(message_id, data_name);
  RoutingMessage message(vault_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, account_name),
                         VaultMessage::Receiver(routing::GroupId(NodeId(data_name.raw_name))));
  routing_.Send(message);
}

void MaidManagerDispatcher::SendCreateAccountResponse(const MaidName& account_name,
                                                      const maidsafe_error& result,
                                                      nfs::MessageId message_id) {
  typedef nfs::CreateAccountResponseFromMaidManagerToMaidNode VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();

  VaultMessage vault_message(message_id, nfs_client::ReturnCode(result));
  RoutingMessage message(vault_message.Serialise(),
      GroupOrKeyHelper::GroupSender(routing_, account_name),
      VaultMessage::Receiver(routing::SingleId(NodeId(account_name.value.string()))));
  routing_.Send(message);
}

void MaidManagerDispatcher::SendRemoveAccountResponse(const MaidName& /*account_name*/,
                                                      const maidsafe_error& /*result*/,
                                                      nfs::MessageId /*message_id*/) {
  //  typedef routing::GroupToSingleMessage RoutingMessage;
  //  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  //  static const nfs::MessageAction kAction(nfs::MessageAction::kRemoveAccountResponse);
  //  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  //  nfs::Message::Data inner_data(result);
  //  inner_data.action = kAction;
  //  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  //  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
  //                         routing::SingleId(NodeId(account_name->string())), cacheable);
  //  routing_.Send(message);
}

void MaidManagerDispatcher::SendUnregisterPmidResponse(const MaidName& /*account_name*/,
                                                       const PmidName& /*pmid_name*/,
                                                       const maidsafe_error& /*result*/,
                                                       nfs::MessageId /*message_id*/) {
  //  typedef routing::GroupToSingleMessage RoutingMessage;
  //  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  //  static const nfs::MessageAction kAction(nfs::MessageAction::kUnregisterPmidResponse);
  //  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  //  nfs::Message::Data inner_data(result);
  //  inner_data.type = PmidName::data_type::Tag::kValue;
  //  inner_data.name = pmid_name.data;
  //  inner_data.action = kAction;
  //  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  //  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
  //                         routing::SingleId(NodeId(account_name->string())), cacheable);
  //  routing_.Send(message);
}

void MaidManagerDispatcher::SendAccountTransfer(const NodeId& /*destination_peer*/,
                                                const MaidName& /*account_name*/,
                                                const std::string& /*serialised_account*/) {
  //  typedef routing::GroupToSingleMessage RoutingMessage;
  //  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  //  static const nfs::MessageAction kAction(nfs::MessageAction::kAccountTransfer);
  //  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidManager);

  //  nfs::Message::Data inner_data;
  //  inner_data.content = NonEmptyString(serialised_account);
  //  inner_data.action = kAction;
  //  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  //  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
  //                         routing::SingleId(destination_peer), cacheable);
  //  routing_.Send(message);
}

void MaidManagerDispatcher::SendHealthResponse(const MaidName& maid_name, int64_t available_size,
    const nfs_client::ReturnCode& return_code, nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerDispatcher::SendHealthResponse for maid "
                << HexSubstr(maid_name->string()) << " . available_size " << available_size
                << " and return code : " << return_code.value.what();
  typedef nfs::PmidHealthResponseFromMaidManagerToMaidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<NfsMessage>();

  NfsMessage nfs_message(message_id, nfs_client::AvailableSizeAndReturnCode(available_size,
                                                                            return_code));
  RoutingMessage message(nfs_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, maid_name),
                         NfsMessage::Receiver(NodeId(maid_name.value.string())));
  routing_.Send(message);
}

void MaidManagerDispatcher::SendCreatePmidAccountRequest(const passport::PublicMaid& account_name,
    const passport::PublicPmid& pmid_name) {
  LOG(kVerbose) << "MaidManagerDispatcher::SendCreatePmidAccountRequest for maid "
                << HexSubstr(account_name.name()->string()) << " create PmidAccount for "
                << HexSubstr(pmid_name.name()->string());
  typedef CreatePmidAccountRequestFromMaidManagerToPmidManager VaultMessage;
  typedef routing::Message<VaultMessage::Sender, VaultMessage::Receiver> RoutingMessage;
  CheckSourcePersonaType<VaultMessage>();

  VaultMessage vault_message(nfs_vault::DataName(DataTagValue::kPmidValue, pmid_name.name().value));
  RoutingMessage message(vault_message.Serialise(),
                         GroupOrKeyHelper::GroupSender(routing_, account_name.name()),
                         VaultMessage::Receiver(NodeId(pmid_name.name()->string())));
  routing_.Send(message);
}

}  // namespace vault

}  // namespace maidsafe
