/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/structured_data_manager/structured_data_manager_service.h"

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/utils.h"

#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/nfs/structured_data.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/structured_data_manager/structured_data_key.h"
#include "maidsafe/vault/structured_data_manager/structured_data_unresolved_entry_value.h"
#include "maidsafe/vault/unresolved_element.pb.h"
#include "maidsafe/vault/manager_db.h"

namespace maidsafe {

namespace vault {

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() == nfs::Persona::kStructuredDataManager;
}

template<typename Message>
inline bool FromStructuredDataManager(const Message& message) {
  return message.destination_persona() == nfs::Persona::kStructuredDataManager;
}

}  // unnamed namespace



namespace detail {

StructuredDataUnresolvedEntry UnresolvedEntryFromMessage(const nfs::Message& message) {
  // test message content is valid only
  protobuf::StructuredDataUnresolvedEntry entry_proto;
  if (!entry_proto.ParseFromString(message.data().content.string()))
    ThrowError(CommonErrors::parsing_error);
  // this is the only code line really required
  return StructuredDataUnresolvedEntry(
                         StructuredDataUnresolvedEntry::serialised_type(message.data().content));

}

}  // namespace detail



StructuredDataManagerService::StructuredDataManagerService(const passport::Pmid& pmid,
                                                           routing::Routing& routing,
                                                           const boost::filesystem::path& path)
    : routing_(routing),
      accumulator_mutex_(),
      sync_mutex_(),
      accumulator_(),
      structured_data_db_(path),
      kThisNodeId_(routing_.kNodeId()),
      sync_(&structured_data_db_, kThisNodeId_),
      nfs_(routing_, pmid) {}


void StructuredDataManagerService::ValidateClientSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedClient(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!(FromClientMaid(message) || FromClientMpid(message)) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void StructuredDataManagerService::ValidateSyncSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedVault(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!FromStructuredDataManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

std::vector<StructuredDataVersions::VersionName>
    StructuredDataManagerService::GetVersionsFromMessage(const nfs::Message& msg) const {
   return nfs::StructuredData(nfs::StructuredData::serialised_type(msg.data().content)).versions();
}

NonEmptyString StructuredDataManagerService::GetSerialisedRecord(
    const StructuredDataManager::DbKey& db_key) {
  protobuf::UnresolvedEntries proto_unresolved_entries;
  auto db_value(structured_data_db_.Get(db_key));
  StructuredDataKey structured_data_key;
  //structured_data_key.
  //StructuredDataUnresolvedEntry unresolved_entry_db_value(
  //    std::make_pair(data_name, nfs::MessageAction::kAccountTransfer), metadata_value,
  //      kThisNodeId_);
  //auto unresolved_data(sync_.GetUnresolvedData(data_name));
  //unresolved_data.push_back(unresolved_entry_db_value);
  //for (const auto& unresolved_entry : unresolved_data) {
  //  proto_unresolved_entries.add_serialised_unresolved_entry(
  //      unresolved_entry.Serialise()->string());
  //}
  //assert(proto_unresolved_entries.IsInitialized());
  return NonEmptyString(proto_unresolved_entries.SerializeAsString());
}


// =============== Get data =================================================================

void StructuredDataManagerService::HandleGet(const nfs::Message& message,
                                             routing::ReplyFunctor reply_functor) {
  try {
    nfs::Reply reply(CommonErrors::success);
    StructuredDataVersions version(
                structured_data_db_.Get(GetKeyFromMessage<StructuredDataManager>(message)));
    reply.data() = nfs::StructuredData(version.Get()).Serialise().data;
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, nfs::Reply(CommonErrors::success));
  }
  catch (std::exception& e) {
    LOG(kError) << "Bad message: " << e.what();
    nfs::Reply reply(VaultErrors::failed_to_handle_request);
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, nfs::Reply(VaultErrors::failed_to_handle_request));
 }
}

void StructuredDataManagerService::HandleGetBranch(const nfs::Message& message,
                                                   routing::ReplyFunctor reply_functor) {

  try {
    nfs::Reply reply(CommonErrors::success);
    StructuredDataVersions version(
                structured_data_db_.Get(GetKeyFromMessage<StructuredDataManager>(message)));
    auto branch_to_get(GetVersionsFromMessage(message));
    reply.data() = nfs::StructuredData(version.GetBranch(branch_to_get.at(0))).Serialise().data;
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, nfs::Reply(CommonErrors::success));
  }
  catch (std::exception& e) {
    LOG(kError) << "Bad message: " << e.what();
    nfs::Reply reply(VaultErrors::failed_to_handle_request);
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, nfs::Reply(VaultErrors::failed_to_handle_request));
 }
}

// // =============== Sync ============================================================================

void StructuredDataManagerService::HandleSynchronise(const nfs::Message& message) {
  std::vector<StructuredDataMergePolicy::UnresolvedEntry> unresolved_entries;
  bool success(false);
  try {
    {
      std::lock_guard<std::mutex> lock(sync_mutex_);
      unresolved_entries = sync_.AddUnresolvedEntry(detail::UnresolvedEntryFromMessage(message));
    }
    success = true;
  } catch (std::exception& e) {
    LOG(kError) << "invalid request" << e.what();
    success = false;
  }
  if (unresolved_entries.size() >= routing::Parameters::node_group_size -1U) {
    for (const auto& entry : unresolved_entries) {
      {
        std::lock_guard<std::mutex> lock(accumulator_mutex_);
        if (success)
          accumulator_.SetHandledAndReply(entry.original_message_id,
                                          entry.source_node_id,
                                          nfs::Reply(CommonErrors::success));
        else
          accumulator_.SetHandledAndReply(entry.original_message_id,
                                          entry.source_node_id,
                                          nfs::Reply(VaultErrors::failed_to_handle_request));
      }
    }
  }
}

void StructuredDataManagerService::HandleChurnEvent(routing::MatrixChange /*matrix_change*/) {
//  auto record_names(structured_data_db_.GetKeys());
//  auto itr(std::begin(record_names));
//  while (itr != std::end(record_names)) {
//    auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), *itr));
//    auto check_holders_result(CheckHolders(matrix_change, routing_.kNodeId(),
//                                           NodeId(result.second)));
//    // Delete records for which this node is no longer responsible.
//    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
//      structured_data_db_.Delete(*itr);
//      itr = record_names.erase(itr);
//      continue;
//    }
//
//    // Replace old_node(s) in sync object and send TransferRecord to new node(s).
//    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
//    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
//      // FIXME(Prakash) Need to pass record_name to sync
//      // or have sync test whenther new node should be managing the 'account'
//      // we can use the routing IsResponsibleFor(account_name, node_name) test I think
//      // This is a bit of work but same test will be required for account transfer
//      sync_.ReplaceNode(/**itr, */check_holders_result.old_holders[i], check_holders_result.new_holders[i]);
//      nfs_.TransferRecord(*itr, check_holders_result.new_holders[i],
//                          metadata_handler_.GetSerialisedRecord(record_name));
//
//      TransferRecord(*itr, check_holders_result.new_holders[i]);
//    }
//    ++itr;
//  }
//  // TODO(Prakash):  modify ReplaceNodeInSyncList to be called once with vector of tuple/struct
//  // containing record name, old_holders, new_holders.
}


//void StructuredDataManagerService::HandleChurnEvent(const NodeId& /*old_node*/,
//                                                    const NodeId& /*new_node*/) {
//    //// for each unresolved entry replace node (only)
//    //{
//    //std::lock_guard<std::mutex> lock(sync_mutex_);
//    //sync_.ReplaceNode(old_node, new_node);
//    //}
//    ////  carry out account transfer for new node !
//    //std::vector<StructuredDataManager::DbKey> db_keys;
//    //db_keys = structured_data_db_.GetKeys();
//    //for (const auto& key: db_keys) {
//    //  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
//    //  if (routing_.IsNodeIdInGroupRange(NodeId(result.second.string()), new_node) ==
//    //      routing::GroupRangeStatus::kInRange) {  // TODO(dirvine) confirm routing method here !!!!!!!!
//    //    // for each db record the new node should have, send it to him (AccountNameFromKey)
//    //  }
//    //}
//}


}  // namespace vault

}  // namespace maidsafe
