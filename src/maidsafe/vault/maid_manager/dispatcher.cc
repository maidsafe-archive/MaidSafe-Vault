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

#include "maidsafe/vault/maid_manager/dispatcher.h"

namespace maidsafe {

namespace vault {

const nfs::Persona MaidManagerDispatcher::kSourcePersona_(nfs::Persona::kMaidManager);

MaidManagerDispatcher::MaidManagerDispatcher(routing::Routing& routing,
                                             const passport::Pmid& signing_fob)
    : routing_(routing),
      kSigningFob_(signing_fob) {}

void MaidManagerDispatcher::SendCreateAccountResponse(const MaidName& account_name,
                                                      const maidsafe_error& result) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kCreateAccountResponse);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  nfs::Message::Data inner_data(result);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(NodeId(account_name->string())), cacheable);
  routing_.Send(message);
}

void MaidManagerDispatcher::SendRemoveAccountResponse(const MaidName& account_name,
                                                      const maidsafe_error& result) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kRemoveAccountResponse);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  nfs::Message::Data inner_data(result);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(NodeId(account_name->string())), cacheable);
  routing_.Send(message);
}

void MaidManagerDispatcher::SendRegisterPmidResponse(const MaidName& account_name,
                                                     const PmidName& pmid_name,
                                                     const maidsafe_error& result) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kRegisterPmidResponse);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  nfs::Message::Data inner_data(result);
  inner_data.type = PmidName::data_type::Tag::kValue;
  inner_data.name = pmid_name.data;
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(NodeId(account_name->string())), cacheable);
  routing_.Send(message);
}

void MaidManagerDispatcher::SendUnregisterPmidResponse(const MaidName& account_name,
                                                       const PmidName& pmid_name,
                                                       const maidsafe_error& result) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kUnregisterPmidResponse);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidNode);

  nfs::Message::Data inner_data(result);
  inner_data.type = PmidName::data_type::Tag::kValue;
  inner_data.name = pmid_name.data;
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(NodeId(account_name->string())), cacheable);
  routing_.Send(message);
}

void MaidManagerDispatcher::SendSync(const NodeId& destination_peer,
                                     const MaidName& account_name,
                                     const std::string& serialised_sync) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kSynchronise);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidManager);

  nfs::Message::Data inner_data;
  inner_data.content = NonEmptyString(serialised_sync);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(destination_peer), cacheable);
  routing_.Send(message);
}

void MaidManagerDispatcher::SendAccountTransfer(const NodeId& destination_peer,
                                                const MaidName& account_name,
                                                const std::string& serialised_account) {
  typedef routing::GroupToSingleMessage RoutingMessage;
  static const routing::Cacheable cacheable(routing::Cacheable::kNone);
  static const nfs::MessageAction kAction(nfs::MessageAction::kAccountTransfer);
  static const nfs::Persona kDestinationPersona(nfs::Persona::kMaidManager);

  nfs::Message::Data inner_data;
  inner_data.content = NonEmptyString(serialised_account);
  inner_data.action = kAction;
  nfs::Message inner(kDestinationPersona, kSourcePersona_, inner_data);
  RoutingMessage message(inner.Serialise()->string(), Sender(account_name),
                         routing::SingleId(destination_peer), cacheable);
  routing_.Send(message);
}

routing::GroupSource MaidManagerDispatcher::Sender(const MaidName& account_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(account_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe
