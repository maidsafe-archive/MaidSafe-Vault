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

#include <string>
#include <vector>

#include "maidsafe/routing/message.h"
#include "maidsafe/routing/node_info.h"

#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/client/messages.h"

#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"


namespace maidsafe {

namespace vault {

namespace test {

static const size_t kTestChunkSize = 2048U;
static const size_t kAverageChunksStored = 1000;

routing::NodeInfo MakeNodeInfo(const passport::Pmid& pmid);

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

template <>
nfs_vault::DataNameAndCost CreateContent<nfs_vault::DataNameAndCost>();

template <>
nfs_vault::DataNameAndVersion CreateContent<nfs_vault::DataNameAndVersion>();

template <>
nfs_vault::DataNameOldNewVersion CreateContent<nfs_vault::DataNameOldNewVersion>();

template <>
nfs_vault::VersionTreeCreation CreateContent<nfs_vault::VersionTreeCreation>();

template <typename MessageType>
MessageType CreateMessage(const typename MessageType::Contents& contents) {
  nfs::MessageId message_id(RandomUint32());
  return MessageType(message_id, contents);
}

template <typename ServiceType, typename MessageType>
void GroupSendToGroup(ServiceType* service, const MessageType& message,
                      const std::vector<routing::GroupSource>& group_sources,
                      const routing::GroupId& group_id) {
  for (const auto& group_source : group_sources)
    service->HandleMessage(message, group_source, group_id);
}

template <typename ServiceType, typename MessageType>
void GroupSendToSingle(ServiceType* service, const MessageType& message,
                       const std::vector<routing::GroupSource>& group_sources,
                       const routing::SingleId& single_id) {
  for (const auto& group_source : group_sources)
    service->HandleMessage(message, group_source, single_id);
}

template <typename ServiceType, typename MessageType>
void SingleSendsToSingle(ServiceType* service, const MessageType& message,
                         const routing::SingleSource& single_source,
                         const routing::SingleId& single_id) {
  service->HandleMessage(message, single_source, single_id);
}

template <typename ServiceType, typename MessageType>
void SingleSendsToGroup(ServiceType* service, const MessageType& message,
                        const routing::SingleSource& single_source,
                        const routing::GroupId& group_id) {
  service->HandleMessage(message, single_source, group_id);
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
  for (const auto& group_source : group_sources)
    unresolved_actions.push_back(UnresolvedActionType(key, action, group_source.sender_id.data));
  return unresolved_actions;
}

template <typename ServiceType, typename UnresolvedActionType, typename PersonaSyncType>
void AddLocalActionAndSendGroupActions(ServiceType* service, Sync<UnresolvedActionType>& /*sync*/,
                                       const std::vector<UnresolvedActionType>& unresolved_actions,
                                       const std::vector<routing::GroupSource>& group_source) {
  for (uint32_t index(0); index < unresolved_actions.size(); ++index) {
    auto proto_sync(CreateProtoSync(UnresolvedActionType::ActionType::kActionId,
                                    unresolved_actions[index].Serialise()));
    auto sync_message(
        CreateMessage<PersonaSyncType>(nfs_vault::Content(proto_sync.SerializeAsString())));
    service->HandleMessage(sync_message, group_source[index], group_source[index].group_id);
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_TESTS_UTILS_H_
