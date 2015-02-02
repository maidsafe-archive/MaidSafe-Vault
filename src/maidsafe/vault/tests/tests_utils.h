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
#include <utility>
#include <vector>

#include "maidsafe/routing/message.h"
#include "maidsafe/routing/node_info.h"

#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/client/messages.h"

#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"

namespace maidsafe {

namespace vault {

namespace test {

static const uint64_t kTestChunkSize = 1024 * 1024;
static const uint64_t kAverageChunksStored = 1000;

routing::NodeInfo MakeNodeInfo(const passport::Pmid& pmid);

template <typename ContentType>
ContentType CreateContent() {
  ContentType::No_genereic_handler_is_available__Specialisation_is_required;
  return ContentType();
}

template <>
nfs_vault::DataNameAndContent CreateContent<nfs_vault::DataNameAndContent>();

template <>
nfs_client::DataNameAndSizeAndSpaceAndReturnCode
    CreateContent<nfs_client::DataNameAndSizeAndSpaceAndReturnCode>();

template <>
nfs_client::DataNameAndSizeAndReturnCode
    CreateContent<nfs_client::DataNameAndSizeAndReturnCode>();

template <>
nfs_vault::DataName CreateContent<nfs_vault::DataName>();

template <>
nfs_vault::AvailableSize CreateContent<nfs_vault::AvailableSize>();

template <>
nfs_vault::Empty CreateContent<nfs_vault::Empty>();

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
nfs_vault::DiffSize CreateContent<nfs_vault::DiffSize>();

template <>
nfs_vault::VersionTreeCreation CreateContent<nfs_vault::VersionTreeCreation>();

template <>
nfs_vault::MpidMessageAlert CreateContent<nfs_vault::MpidMessageAlert>();

template <>
nfs_vault::MpidMessage CreateContent<nfs_vault::MpidMessage>();

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

template <typename ServiceType, typename MessageType>
void SingleRelaySendsToGroup(ServiceType* service, const MessageType& message,
                        const routing::SingleRelaySource& single_source,
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

template <typename Persona>
typename Persona::Value CreateValue();

template <>
MaidManager::Value CreateValue<MaidManager>();

template <>
typename PmidManager::Value CreateValue<PmidManager>();

template <typename Persona, typename AddResult>
struct ApplyMedian {};

template <typename AddResult>
struct ApplyMedian<MaidManager, AddResult> {
  std::pair<AddResult, boost::optional<MaidManager::Value>> operator()(
      const std::vector<std::pair<MaidManager::Key, MaidManager::Value>>& pairs) {
    std::vector<uint64_t> data_stored, space_available;
    for (const auto& pair : pairs) {
      data_stored.emplace_back(pair.second.data_stored);
      space_available.emplace_back(pair.second.space_available);
    }

    MaidManager::Value value;
    value.data_stored = Median(data_stored);
    value.space_available = Median(space_available);

    return std::make_pair(AddResult::kSuccess, boost::optional<MaidManager::Value>(value));
  }
};

template <typename AddResult>
struct ApplyMedian<PmidManager, AddResult> {
  std::pair<AddResult, boost::optional<PmidManager::Value>> operator()(
      const std::vector<std::pair<PmidManager::Key, PmidManager::Value>>& pairs) {
    std::vector<int64_t> stored_total_size, lost_total_size, offered_space;
    for (const auto& value : pairs) {
      stored_total_size.emplace_back(value.second.stored_total_size);
      lost_total_size.emplace_back(value.second.lost_total_size);
      offered_space.emplace_back(value.second.offered_space);
    }

    PmidManager::Value value;
    value.stored_total_size = Median(stored_total_size);
    value.lost_total_size = Median(lost_total_size);
    value.offered_space = Median(offered_space);

    return std::make_pair(AddResult::kSuccess, boost::optional<PmidManager::Value>(value));
  }
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_TESTS_UTILS_H_
