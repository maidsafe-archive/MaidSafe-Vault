/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_TESTS_TESTS_UTILS_H_
#define MAIDSAFE_VAULT_TESTS_TESTS_UTILS_H_

#include "maidsafe/routing/message.h"

#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/client/messages.h"

#include "maidsafe/vault/sync.pb.h"


namespace maidsafe {

namespace vault {

namespace test {

static const int TEST_CHUNK_SIZE = 2^10;

passport::Maid MakeMaid();
passport::Pmid MakePmid();
passport::PublicPmid MakePublicPmid();

template <typename ContentType>
ContentType CreateContent() {
  ContentType::No_genereic_handler_is_available__Specialisation_is_required;
  return ContentType();
}

template <>
nfs_vault::DataNameAndContent CreateContent<nfs_vault::DataNameAndContent>();

template <>
nfs_client::DataNameAndSpaceAndReturnCode
CreateContent<nfs_client::DataNameAndSpaceAndReturnCode>();

template <>
nfs_vault::DataName CreateContent<nfs_vault::DataName>();

template <>
nfs_vault::AvailableSize CreateContent<nfs_vault::AvailableSize>();

template <>
nfs_vault::Empty CreateContent<nfs_vault::Empty>();

template <>
nfs_vault::DataAndPmidHint CreateContent<nfs_vault::DataAndPmidHint>();

template <>
nfs_vault::DataNameAndSize CreateContent<nfs_vault::DataNameAndSize>();

template <>
nfs_client::DataNameAndReturnCode CreateContent<nfs_client::DataNameAndReturnCode>();

template <>
nfs_vault::DataNameAndContentOrCheckResult
CreateContent<nfs_vault::DataNameAndContentOrCheckResult>();

template <>
nfs_client::DataNameAndContentOrReturnCode
CreateContent<nfs_client::DataNameAndContentOrReturnCode>();

template <typename MessageType>
MessageType CreateMessage(const typename MessageType::Contents& contents) {
  nfs::MessageId message_id(RandomUint32());
  return MessageType(message_id, contents);
}

template <typename ServiceType, typename MessageType>
void GroupSendToGroup(ServiceType* service, const MessageType& message,
                      const std::vector<routing::GroupSource>& group_sources,
                      const routing::GroupId& group_id) {
  for (u_int32_t index(0); index < group_sources.size(); ++index)
    service->HandleMessage(message, group_sources[index], group_id);
}

template <typename DataNameType>
std::vector<routing::GroupSource> CreateGroupSource(const DataNameType& data_name) {
  return CreateGroupSource(NodeId(data_name.value.string()));
}

template <>
std::vector<routing::GroupSource> CreateGroupSource(const NodeId& group_id);


protobuf::Sync CreateProtoSync(nfs::MessageAction action_type,
                               const std::string& serialised_action);

template <typename UnresolvedActionType>
std::vector<UnresolvedActionType> CreateGroupUnresolvedAction(
    const typename UnresolvedActionType::KeyType& key,
    const typename UnresolvedActionType::ActionType& action,
    const std::vector<routing::GroupSource>& group_sources) {
  std::vector<UnresolvedActionType> unresolved_actions;
  for (uint32_t index(0); index < group_sources.size(); ++index)
    unresolved_actions.push_back(UnresolvedActionType(key, action,
                                                      group_sources.at(index).sender_id.data));
  return unresolved_actions;
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_TESTS_UTILS_H_
