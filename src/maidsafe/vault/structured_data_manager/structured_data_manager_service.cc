/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

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
#include "maidsafe/vault/structured_data_manager/structured_data_value.h"
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

   return nfs::StructuredData(nfs::StructuredData::serialised_type(msg.data().content)).Versions();
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


void StructuredDataManagerService::HandleChurnEvent(const NodeId& old_node,
                                                    const NodeId& new_node) {
    // for each unresolved entry replace node (only)
    {
    std::lock_guard<std::mutex> lock(sync_mutex_);
    sync_.ReplaceNode(old_node, new_node);
    }
    //  carry out account transfer for new node !
    std::vector<StructuredDataManager::DbKey> db_keys;
    db_keys = structured_data_db_.GetKeys();
    for (const auto& key: db_keys) {
      auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
      if (routing_.IsNodeIdInGroupRange(NodeId(result.second.string()), new_node) ==
          routing::GroupRangeStatus::kInRange) {  // TODO(dirvine) confirm routing method here !!!!!!!!
        // for each db record the new node should have, send it to him (AccountNameFromKey)
      }
    }
}


}  // namespace vault

}  // namespace maidsafe
