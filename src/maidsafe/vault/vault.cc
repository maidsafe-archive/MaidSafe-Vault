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

#include "maidsafe/vault/vault.h"

#include <algorithm>
#include <vector>

#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "boost/lexical_cast.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/private/chunk_actions/chunk_action_authority.h"
#include "maidsafe/private/chunk_store/buffered_chunk_store.h"
#include "maidsafe/private/chunk_store/chunk_manager.h"

#include "maidsafe/pd/common/aba_messages_pb.h"
#include "maidsafe/pd/common/account_messages_pb.h"
#include "maidsafe/pd/common/admin_messages_pb.h"
#include "maidsafe/pd/common/chunk_info_messages_pb.h"
#include "maidsafe/pd/common/chunk_messages_pb.h"
#include "maidsafe/pd/common/config.h"
#include "maidsafe/pd/common/local_key_manager.h"
#include "maidsafe/pd/common/message_handler.h"
#include "maidsafe/pd/common/return_codes.h"
#include "maidsafe/pd/common/rpc_handler.h"
#include "maidsafe/pd/common/sync_messages_pb.h"
#include "maidsafe/pd/common/timed_event_handler.h"
#include "maidsafe/pd/common/utils.h"

#include "maidsafe/pd/vault/aba_service.h"
#include "maidsafe/pd/vault/account_handler.h"
#include "maidsafe/pd/vault/account_service.h"
#include "maidsafe/pd/vault/chunk_info_handler.h"
#include "maidsafe/pd/vault/chunk_info_service.h"
#include "maidsafe/pd/vault/chunk_integrity_manager.h"
#include "maidsafe/pd/vault/chunk_service.h"
#ifndef __APPLE__
#  include "maidsafe/pd/vault/vault_stats.h"
#endif


namespace bptime = boost::posix_time;

namespace maidsafe {

namespace vault {

Node::Node()
    : pd::Node(),
      state_(NodeState::kStopped),
      vault_stats_needed_(false),
      do_prune_expired_data_(true),
      do_backup_state_(true),
      do_synchronise_(true),
      do_check_integrity_(true),
      do_announce_chunks_(true),
      do_update_account_(true),
      plugins_path_(),
      last_backup_hashes_(),
      prev_close_nodes_(),
      account_capacity_(0),
      start_time_(bptime::pos_infin),
      mutex_(),
      local_chunks_(),
      chunk_info_handler_(),
      account_handler_(),
#ifndef __APPLE__
      vault_stats_(),
#endif
      chunk_service_(),
      chunk_info_service_(),
      account_service_(),
      aba_service_(),
      chunk_integrity_manager_() {}

Node::~Node() {
  Stop();
}

void Node::PrepareServices() {
  pd::Node::PrepareServices();

  chunk_info_handler_.reset(new ChunkInfoHandler(chunk_action_authority_));
  account_handler_.reset(new AccountHandler());

  chunk_integrity_manager_.reset(new ChunkIntegrityManager(
      NodeId(fob_.identity.string()), chunk_info_handler_, chunk_manager_, rpc_handler_));

  // Restore any saved state information
  RestoreState();

#ifndef __APPLE__
  if (vault_stats_needed_) {
    // Use first 10 digits of pmid for vault stats
    std::string id("vlt_" + EncodeToBase32(fob_.identity).substr(0, 10));
    LOG(kInfo) << "PrepareServices - ID for vault stats: " << id;
    vault_stats_.reset(new VaultStats(
        "VaultSharedMemory-" + id, id, timed_event_handler_, bptime::minutes(2), chunk_store_,
        chunk_info_handler_, plugins_path_));
    if (vault_stats_->Init())
      vault_stats_->Start();
  }
#endif

  // initialise services
  chunk_service_.reset(new ChunkService(
      timed_event_handler_, fob_, account_name_, chunk_action_authority_, key_manager_,
      chunk_store_, chunk_manager_, rpc_handler_));
  chunk_info_service_.reset(new ChunkInfoService(
      fob_, chunk_action_authority_, key_manager_, chunk_info_handler_, chunk_store_,
      rpc_handler_));
  account_service_.reset(new AccountService(
      HexSubstr(fob_.identity), chunk_action_authority_, key_manager_, account_handler_));
  aba_service_.reset(new AbaService(
      HexSubstr(fob_.identity), chunk_action_authority_, chunk_store_));

  // Handle update of closest nodes
  message_handler_->set_on_close_node_replaced(
      [this] (const std::vector<NodeId> &new_close_nodes) {
        OnCloseNodeReplaced(new_close_nodes);
      });
}

int Node::Start(const boost::filesystem::path &chunk_path,
                const std::vector<boost::asio::ip::udp::endpoint> &peer_endpoints) {
  if (!fob_.identity.IsInitialised()) {
    LOG(kError) << "Start - No keys, vault can't be anonymous.";
    return kGeneralError;
  }

  LOG(kInfo) << "Starting vault node " << HexSubstr(fob_.identity) << " with account "
             << HexSubstr(account_name_) << " ...";
  {
    boost::mutex::scoped_lock lock(mutex_);
    if (state_ != NodeState::kStopped) {
      LOG(kWarning) << "Start - " << HexSubstr(fob_.identity) << " - Already started.";
      return kSuccess;
    }

    state_ = NodeState::kStarting;
    int result = Init(chunk_path, peer_endpoints, false);
    if (result != kSuccess) {
      LOG(kError) << "Start - Failed to initialise node. (" << result << ")";
      state_ = NodeState::kStopped;
      return result;
    }
    state_ = NodeState::kStarted;
    start_time_ = bptime::microsec_clock::universal_time();
  }

  // Handle incoming messages
  message_handler_->set_on_message_received(
      [this] (const int32_t &message_id,
              const std::string &message,
              const Contact &sender,
              const NodeId &group_claim,
              const SecurityType &security_type,
              bool cache_lookup,
              const ReplyFunctor &reply_functor) {
        // process request in separate thread to be nice to transport layers
        timed_event_handler_->Post([=] {
#ifdef NDEBUG
          try {
#endif
            OnMessageReceived(static_cast<RpcMessageType>(message_id), message, sender, group_claim,
                              security_type, cache_lookup, reply_functor);
#ifdef NDEBUG
          }
          catch(const std::exception &ex) {
            LOG(kError) << "Exception in OnMessageReceived: " << ex.what();
          }
#endif
        });
      });

  // Handle caching
  message_handler_->set_on_store_cache_data(
      [this] (const int32_t &message_id, const std::string &message) {
        // process request in separate thread to be nice to transport layers
        timed_event_handler_->Post([=] {
#ifdef NDEBUG
          try {
#endif
            OnStoreCacheData(static_cast<RpcMessageType>(-message_id), message);
#ifdef NDEBUG
          }
          catch(const std::exception &ex) {
            LOG(kError) << "Exception in OnStoreCacheData: " << ex.what();
          }
#endif
        });
      });

  if (do_update_account_) {
    if (!account_name_.IsInitialised()) {
      LOG(kError) << "Start - " << HexSubstr(fob_.identity) << " - Account name not set.";
      return kGeneralError;
    }
    bool done(false), success(false);
    boost::mutex mutex;
    boost::condition_variable cond_var;
    UpdateAccount(
        true, [&](bool result) {
          boost::mutex::scoped_lock lock(mutex);
          done = true;
          success = result;
          cond_var.notify_one();
        });
    boost::mutex::scoped_lock lock(mutex);
    if (!cond_var.timed_wait(lock, bptime::minutes(1), [&done]() { return done; })) {
      LOG(kError) << "Start - " << HexSubstr(fob_.identity) << " - Updating account timed out.";
      return kGeneralError;
    }
    if (!success) {
      LOG(kError) << "Start - " << HexSubstr(fob_.identity) << " - Failed to update account.";
      return kGeneralError;
    }
  }

  PerformMaintenance();

  if (do_backup_state_)
    BackupState();

  if (do_announce_chunks_) {
    local_chunks_ = chunk_store_->GetChunks();
    if (!local_chunks_.empty()) {
      LOG(kInfo) << "Start - " << HexSubstr(fob_.identity) << " - Going to announce "
                 << local_chunks_.size() << " local chunks.";
      AnnounceChunk();
      AnnounceChunk();
      AnnounceChunk();
    }
  }

  return kSuccess;
}

int Node::Stop() {
  {
    boost::mutex::scoped_lock lock(mutex_);
    if (state_ == NodeState::kStopped || state_ == NodeState::kStopping) {
//       LOG(kWarning) << "Stop - " << HexSubstr(fob_.identity) << " - Not started.";
      return kSuccess;
    }
    state_ = NodeState::kStopping;
    start_time_ = bptime::pos_infin;
  }

  if (timed_event_handler_)
    timed_event_handler_->CancelAll();
  if (message_handler_)
    message_handler_->set_on_message_received(nullptr);

  chunk_service_.reset();
  chunk_info_service_.reset();
  account_service_.reset();
  aba_service_.reset();

#ifndef __APPLE__
  try {
    if (vault_stats_)
      vault_stats_->Stop();
  } catch(const std::exception &e) {
    LOG(kError) << "Stop - Error while shutting down vault stats: " << e.what();
    return kGeneralError;
  }
#endif

  // TODO(Steve) wait for pending RPCs

  BackupState();  // final backup of data

  auto result = pd::Node::Stop();
  state_ = NodeState::kStopped;
  return result;
}

void Node::OnMessageReceived(const RpcMessageType &message_type,
                             const std::string &message,
                             const Contact &sender,
                             const NodeId &group_claim,
                             const SecurityType &security_type,
                             bool cache_lookup,
                             const ReplyFunctor &reply_functor) {
  LOG(kVerbose) << "OnMessageReceived - " << HexSubstr(fob_.identity)
                << " - Received message type " << message_type << " from "
                << DebugNodeId(sender.pmid);

  {
    boost::mutex::scoped_lock lock(mutex_);
    if (state_ != NodeState::kStarted) {
       LOG(kError) << "OnMessageReceived - " << HexSubstr(fob_.identity) << " - Not started.";
       return;
    }
  }

  auto respond = [this, message_type, sender, reply_functor](const std::string &response) {
    if (reply_functor) {
      if (response.empty())
        LOG(kWarning) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                      << " - Responding to message type " << message_type << " from "
                      << DebugNodeId(sender.pmid) << " with empty message.";
      else
        LOG(kVerbose) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                      << " - Responding to message type " << message_type << " from "
                      << DebugNodeId(sender.pmid);
      reply_functor(response,
                    SecurityType(kCompressResponse.find(message_type) != kCompressResponse.end(),
                                kEncryptResponse.find(message_type) != kEncryptResponse.end(),
                                kSignResponse.find(message_type) != kSignResponse.end()));
    } else {
      LOG(kWarning) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                    << " - Can't respond to message type " << message_type << " from "
                    << DebugNodeId(sender.pmid) << ", empty reply functor.";
    }
  };

  if (sender.pmid.IsZero() && !kAnonRequest.count(message_type)) {
    LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                << " - Request for RPC " << message_type << " can't be anonymous.";
    return respond("");
  }

  if (kCompressRequest.count(message_type) && !security_type.compression) {
    LOG(kWarning) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                  << " - Request for RPC " << message_type << " from " << DebugNodeId(sender.pmid)
                  << " is not compressed but should be.";
    // return respond("");
  }

  if (kEncryptRequest.count(message_type) && !security_type.encryption) {
    LOG(kWarning) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                  << " - Request for RPC " << message_type << " from " << DebugNodeId(sender.pmid)
                  << " is not encrypted but should be.";
    // return respond("");
  }

  if (kSignRequest.count(message_type) && !security_type.signature) {
    LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                << " - Request for RPC " << message_type << " from " << DebugNodeId(sender.pmid)
                << " has no valid signature.";
    return respond("");
  }

  if (cache_lookup && message_type != RpcMessageType::kChunkInfoGetCached) {
    LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                << " - Invalid cache lookup request for RPC " << message_type;
    return respond("");
  }

  switch (message_type) {
    case RpcMessageType::kChunkArrangeStore: {
      protobuf::chunk::ArrangeStoreRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::ArrangeStoreResponse rsp;
        chunk_service_->ArrangeStore(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkStore: {
      protobuf::chunk::StoreRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::StoreResponse rsp;
        chunk_service_->Store(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkModify: {
      protobuf::chunk::ModifyRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::ModifyResponse rsp;
        chunk_service_->Modify(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkGet: {
      protobuf::chunk::GetRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::GetResponse rsp;
        chunk_service_->Get(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkHas: {
      protobuf::chunk::HasRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::HasResponse rsp;
        chunk_service_->Has(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkValidate: {
      protobuf::chunk::ValidateRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::ValidateResponse rsp;
        chunk_service_->Validate(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkDelete: {
      protobuf::chunk::DeleteRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::DeleteResponse rsp;
        chunk_service_->Delete(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkDuplicate: {
      protobuf::chunk::DuplicateRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk::DuplicateResponse rsp;
        chunk_service_->Duplicate(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoAddReference: {
      protobuf::chunk_info::AddReferenceRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::AddReferenceResponse rsp;
        chunk_info_service_->AddReference(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoRemoveReference: {
      protobuf::chunk_info::RemoveReferenceRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::RemoveReferenceResponse rsp;
        chunk_info_service_->RemoveReference(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoAddInstance: {
      protobuf::chunk_info::AddInstanceRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::AddInstanceResponse rsp;
        chunk_info_service_->AddInstance(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoGetInstances: {
      protobuf::chunk_info::GetInstancesRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::GetInstancesResponse rsp;
        chunk_info_service_->GetInstances(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoAmendVersion: {
      protobuf::chunk_info::AmendVersionRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::AmendVersionResponse rsp;
        chunk_info_service_->AmendVersion(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoActionSynchronisation: {
      protobuf::chunk_info::ActionSynchronisationRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::ActionSynchronisationResponse rsp;
        chunk_info_service_->ActionSynchronisation(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoRemoteAction: {
      protobuf::chunk_info::RemoteActionRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::chunk_info::RemoteActionResponse rsp;
        chunk_info_service_->RemoteAction(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kChunkInfoGetCached: {
      protobuf::chunk_info::GetCachedRequest req;
      if (!rpc_handler_->ParseMessage(message, &req))
        break;
      return chunk_info_service_->GetCached(
          sender, req, cache_lookup,
          [this, cache_lookup, respond] (const protobuf::chunk_info::GetCachedResponse &rsp) {
            if (!cache_lookup || (rsp.has_content() && !rsp.content().empty()))
              respond(rpc_handler_->SerialiseMessage(rsp));
            else
              respond("");
          });
      break;
    }
    case RpcMessageType::kAccountAmend: {
      protobuf::account::AmendRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::account::AmendResponse rsp;
        account_service_->Amend(sender, group_claim, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAccountExpectAmendment: {
      protobuf::account::ExpectAmendmentRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::account::ExpectAmendmentResponse rsp;
        account_service_->ExpectAmendment(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAccountStatus: {
      protobuf::account::StatusRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::account::StatusResponse rsp;
        account_service_->Status(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kSyncGetSyncData: {
      protobuf::sync::GetSyncDataRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        if (group_claim != sender.pmid) {
          LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                      << " - Received GetSyncDataRequest not from group member.";
          break;
        }
        protobuf::sync::GetSyncDataResponse rsp;
        GetSyncData(req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAdminGetProperties: {
      protobuf::admin::GetPropertiesRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        if (sender.pmid.string() != message_handler_->fob().identity.string()) {
          LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                      << " - Received GetPropertiesRequest not from self.";
          break;
        }
        protobuf::admin::GetPropertiesResponse rsp;
        GetProperties(req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAdminExecute: {
      protobuf::admin::ExecuteRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        if (sender.pmid.string() != message_handler_->fob().identity.string()) {
          LOG(kError) << "OnMessageReceived - " << HexSubstr(message_handler_->fob().identity)
                      << " - Received ExecuteRequest not from self.";
          break;
        }
        protobuf::admin::ExecuteResponse rsp;
        Execute(req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAbaStore: {
      protobuf::aba::StoreRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::aba::StoreResponse rsp;
        aba_service_->Store(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAbaModify: {
      protobuf::aba::ModifyRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::aba::ModifyResponse rsp;
        aba_service_->Modify(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAbaGet: {
      protobuf::aba::GetRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::aba::GetResponse rsp;
        aba_service_->Get(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    case RpcMessageType::kAbaDelete: {
      protobuf::aba::DeleteRequest req;
      if (rpc_handler_->ParseMessage(message, &req)) {
        protobuf::aba::DeleteResponse rsp;
        aba_service_->Delete(sender, req, &rsp);
        return respond(rpc_handler_->SerialiseMessage(rsp));
      }
      break;
    }
    default:
      LOG(kWarning) << "OnMessageReceived - " << HexSubstr(fob_.identity)
                    << " - Received unknown message type " << message_type << " from "
                    << DebugNodeId(sender.pmid);
      break;
  }

  respond("");
}

void Node::OnStoreCacheData(const RpcMessageType &message_type, const std::string &message) {
  {
    boost::mutex::scoped_lock lock(mutex_);
    if (state_ != NodeState::kStarted) {
       LOG(kError) << "OnStoreCacheData - " << HexSubstr(fob_.identity) << " - Not started.";
       return;
    }
  }

  if (message_type != RpcMessageType::kChunkInfoGetCached) {
    LOG(kError) << "OnStoreCacheData - " << HexSubstr(fob_.identity)
                << " - Unsupported message type " << message_type;
    return;
  }

  protobuf::chunk_info::GetCachedResponse rsp;
  if (!rpc_handler_->ParseMessage(message, &rsp)) {
    LOG(kError) << "OnStoreCacheData - " << HexSubstr(fob_.identity)
                << " - Failed to parse GetCachedResponse.";
    return;
  }

  if (!rsp.result() || !rsp.has_content() || rsp.content().empty()) {
    LOG(kError) << "OnStoreCacheData - " << HexSubstr(fob_.identity)
                << " - No chunk content to cache.";
    return;
  }

  NonEmptyString content(rsp.content());
  ChunkName name(crypto::Hash<crypto::SHA512>(content));
  if (!chunk_store_->CacheStore(name, content)) {
    LOG(kError) << "OnStoreCacheData - " << HexSubstr(fob_.identity)
                << " - Failed to cache chunk " << DebugChunkName(name);
    return;
  }

  LOG(kSuccess) << "OnStoreCacheData - " << HexSubstr(fob_.identity) << " - Cached chunk "
                << DebugChunkName(name);
}

void Node::OnCloseNodeReplaced(const std::vector<NodeId> &new_close_nodes) {
  LOG(kVerbose) << "OnCloseNodeReplaced - " << HexSubstr(fob_.identity) << " - Got "
                << new_close_nodes.size() << " close nodes.";

  if (chunk_integrity_manager_)
    chunk_integrity_manager_->set_close_nodes(new_close_nodes);

  if (new_close_nodes.size() < 8)
    return;

  std::vector<NodeId> need_sync;
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    // check if any of the new first 4 were not in the previous first 4 (k)
    for (size_t i = 0; i < new_close_nodes.size() && i < 4; ++i) {
      bool found(false);
      for (size_t j = 0; j < prev_close_nodes_.size() && j < 4 && !found; ++j)
        if (new_close_nodes[i] == prev_close_nodes_[j])
          found = true;
      if (!found)
        need_sync.push_back(new_close_nodes[i]);  // we haven't had this one before
    }

    prev_close_nodes_ = new_close_nodes;

    if (!do_synchronise_)
      return;

    if (state_ != NodeState::kStarted && state_ != NodeState::kStarting) {
      LOG(kError) << "OnCloseNodeReplaced - " << HexSubstr(fob_.identity) << " - Not started.";
      return;
    }
  }

  for (auto &id : need_sync) {
    LOG(kInfo) << "OnCloseNodeReplaced - " << HexSubstr(fob_.identity)
                << " - Synchronising with new node " << DebugNodeId(id);
    rpc_handler_->GetSyncData(
        id, [this, id] (bool result, const std::multimap<std::string, std::string> &sync_data) {
          if (result) {
            LOG(kInfo) << "GetSyncDataCallback - " << HexSubstr(fob_.identity)
                        << " - Got " << sync_data.size() << " data sets from "
                        << DebugNodeId(id);
            for (auto &sd : sync_data) {
              if (sd.first == "AH")
                account_handler_->ParseAccounts(sd.second);
              else if (sd.first == "CIH")
                chunk_info_handler_->ParseChunkInfo(sd.second);
              else
                LOG(kWarning) << "GetSyncDataCallback - " << HexSubstr(fob_.identity)
                              << " - Unknown type " << sd.first << " in response from "
                              << DebugNodeId(id);
            }
          } else {
            LOG(kError) << "GetSyncDataCallback - " << HexSubstr(fob_.identity)
                        << " - Failed to synchronise with " << DebugNodeId(id);
          }
        });
  }
}

void Node::PerformMaintenance() {
  if (!fob_.identity.IsInitialised())
    return;
  LOG(kVerbose) << "PerformMaintenance - " << HexSubstr(fob_.identity);

  if (do_prune_expired_data_) {
    NodeId own_id;
    std::vector<NodeId> close_nodes;
    PruneFunctor prune_func = [](const NodeId&) { return false; };  // NOLINT
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      if (prev_close_nodes_.size() >= 8) {
        own_id = NodeId(fob_.identity.string());
        close_nodes = prev_close_nodes_;
        prune_func = [&own_id, &close_nodes](const NodeId &data_id)->bool {
          // return true if we're not within the 4 closest nodes to the data and should prune
          std::sort(close_nodes.begin(), close_nodes.end(),
                    [&data_id] (const NodeId &id1, const NodeId &id2) {
                      return NodeId::CloserToTarget(id1, id2, data_id);
                    });
          return NodeId::CloserToTarget(close_nodes[3], own_id, data_id);
        };
      }
    }
    if (chunk_info_handler_)
      chunk_info_handler_->PruneExpiredData(prune_func);
    if (account_handler_)
      account_handler_->PruneExpiredData(prune_func);
    if (chunk_service_)
      chunk_service_->PruneExpiredData();
    if (account_service_)
      account_service_->PruneExpiredData();
    if (chunk_integrity_manager_)
      chunk_integrity_manager_->PruneExpiredData();
  }

  if (do_check_integrity_)
    chunk_integrity_manager_->TriggerChecks();

  // Schedule next maintenance
  boost::mutex::scoped_lock lock(mutex_);
  if (state_ == NodeState::kStarted &&
      (do_prune_expired_data_ || do_check_integrity_)
      && timed_event_handler_)
    timed_event_handler_->Post([this] { PerformMaintenance(); }, kMaintenanceInterval);  // NOLINT
}

void Node::BackupState() {
  if (!chunk_info_handler_ || !account_handler_ || !chunk_integrity_manager_ ||
      !fob_.identity.IsInitialised() || !timed_event_handler_)
    return;
  LOG(kVerbose) << "BackupState - " << HexSubstr(fob_.identity);

  auto backup_state = [this](const NonEmptyString &name, const std::string &content) {
    if (content.empty())
      return;
    auto const hash = crypto::Hash<crypto::Tiger>(content);
    auto it = last_backup_hashes_.find(name);
    if (it == last_backup_hashes_.end() || hash != it->second) {
      try {
        const ChunkName chunk_name(priv::ApplyTypeToName(
            crypto::Hash<crypto::SHA512>(fob_.identity.string() + name.string()),
            priv::ChunkType::kUnknown));
        const NonEmptyString chunk_content(asymm::Encrypt(
            asymm::PlainText(crypto::Compress(crypto::UncompressedText(content), 9).string()),
            fob_.keys.public_key));
        chunk_store_->Delete(chunk_name);
        if (!chunk_store_->Store(chunk_name, chunk_content))
          throw std::runtime_error("Failed to store.");
        last_backup_hashes_[name] = hash;
        LOG(kInfo) << "BackupState - " << HexSubstr(fob_.identity) << " - Backed up "
                   << name.string() << " data of "
                   << BytesToBinarySiUnits(chunk_content.string().size());
      }
      catch(const std::exception& ex) {
        LOG(kError) << "BackupState - " << HexSubstr(fob_.identity) << " - Failed to backup "
                    << name.string() << " data. (" << ex.what() << ")";
      }
    }
  };

  // Backup chunk infos
  backup_state(NonEmptyString("CIH"), chunk_info_handler_->SerialiseChunkInfo());

  // Backup accounts
  backup_state(NonEmptyString("AH"), account_handler_->SerialiseAccounts());

  // Backup integrity check data
  backup_state(NonEmptyString("CIM"), chunk_integrity_manager_->Serialise());

  // Schedule next backup
  boost::mutex::scoped_lock lock(mutex_);
  if (state_ == NodeState::kStarted && do_backup_state_)
    timed_event_handler_->Post([this] { BackupState(); }, kBackupInterval);  // NOLINT
}

void Node::RestoreState() {
  LOG(kVerbose) << "RestoreState - " << HexSubstr(fob_.identity);

  auto restore_state = [this](const NonEmptyString &name)->std::string {
    const ChunkName chunk_name(priv::ApplyTypeToName(
        crypto::Hash<crypto::SHA512>(fob_.identity.string() + name.string()),
        priv::ChunkType::kUnknown));
    const std::string chunk_content(chunk_store_->Get(chunk_name));
    if (chunk_content.empty())
      return "";
    try {
      auto content = crypto::Uncompress(crypto::CompressedText(asymm::Decrypt(
          crypto::CipherText(chunk_content), fob_.keys.private_key)));
      last_backup_hashes_[name] = crypto::Hash<crypto::Tiger>(content.string());
      LOG(kInfo) << "RestoreState - " << HexSubstr(fob_.identity) << " - Restored " << name.string()
                 << " data of " << BytesToBinarySiUnits(chunk_content.size());
      return content.string();
    }
    catch(const std::exception& ex) {
      LOG(kError) << "RestoreState - " << HexSubstr(fob_.identity) << " - Failed to restore "
                  << name.string() << " data. (" << ex.what() << ")";
      return "";
    }
  };

  // Restore chunk infos
  std::string data = restore_state(NonEmptyString("CIH"));
  if (!data.empty())
    chunk_info_handler_->ParseChunkInfo(data);

  // Restore accounts
  data = restore_state(NonEmptyString("AH"));
  if (!data.empty())
    account_handler_->ParseAccounts(data);

  // Restore integrity check data
  data = restore_state(NonEmptyString("CIM"));
  if (!data.empty())
    chunk_integrity_manager_->Parse(data);
}

void Node::AnnounceChunk() {
  priv::chunk_store::ChunkData chunk_data;
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (state_ != NodeState::kStarted || local_chunks_.empty())
      return;
    chunk_data = local_chunks_.back();
    local_chunks_.pop_back();
  }

  if (!chunk_action_authority_->ValidName(chunk_data.chunk_name)) {
    LOG(kWarning) << "AnnounceChunk - " << HexSubstr(fob_.identity)
                  << " - Skipping invalid chunk " << DebugChunkName(chunk_data.chunk_name);
    return AnnounceChunk();
  }

  auto version = chunk_action_authority_->Version(chunk_data.chunk_name);
  rpc_handler_->AddInstance(  // TODO(Steve) on identity change pass account name and transfer data
      chunk_data.chunk_name, version, chunk_data.chunk_size, AccountName(), "", SignatureData(),
      [=] (bool result) {
        if (!result) {
          LOG(kWarning) << "AnnounceChunk - " << HexSubstr(fob_.identity)
                        << " - Failed to refresh instance of "
                        << DebugChunkName(chunk_data.chunk_name) << " in version "
                        << HexSubstr(version.string());
          chunk_store_->MarkForDeletion(chunk_data.chunk_name);
        }
        AnnounceChunk();
      });
}

void Node::UpdateAccount(bool retry, const std::function<void(bool)> &callback) {  // NOLINT
  LOG(kVerbose) << "UpdateAccount - " << HexSubstr(fob_.identity);
  {
    boost::mutex::scoped_lock lock(mutex_);
    if (state_ != NodeState::kStarted) {
       LOG(kError) << "UpdateAccount - " << HexSubstr(fob_.identity) << " - Not started.";
       return;
    }
  }

  int64_t size(chunk_store_->Size()), capacity(chunk_store_->Capacity());
  const int64_t min_size = 10 * 1024 * 1024;  // 10 MB
  const int64_t size_plus_min_size(size + min_size);
  if (size_plus_min_size > capacity) {
    LOG(kWarning) << "UpdateAccount - " << HexSubstr(fob_.identity)
                  << " - Not enough free space in chunk store, used " << BytesToBinarySiUnits(size)
                  << " of " << BytesToBinarySiUnits(capacity) << ".";
  }

  auto reschedule_and_respond = [this, callback](bool result) {
    { // Schedule next account update
      boost::mutex::scoped_lock lock(mutex_);
      if (state_ == NodeState::kStarted && do_update_account_)
        timed_event_handler_->Post([this] { UpdateAccount(false, nullptr); },
                                   kAccountCapacityUpdateInterval);
    }
    if (callback)
      callback(result);
  };

  if (abs(account_capacity_ - capacity) > kMinAccountCapacityDelta) {
    account_capacity_ = capacity;
    rpc_handler_->Amend(
        account_name_, RpcAccountAmendmentType::kSetOffered, capacity, NodeId(fob_.identity), "",
        NodeId(), [=](bool result) {
          if (result) {
            LOG(kInfo) << "UpdateAccount - " << HexSubstr(fob_.identity)
                      << " - Set space offered to " << BytesToBinarySiUnits(capacity)
                      << " in account " << HexSubstr(account_name_);
          } else if (retry) {
            LOG(kWarning) << "UpdateAccount - " << HexSubstr(fob_.identity)
                          << " - Could not set space offered to " << BytesToBinarySiUnits(capacity)
                          << " in account " << HexSubstr(account_name_) << ", will retry in "
                          << kRpcRetryDelay;
            return timed_event_handler_->Post([this, callback] { UpdateAccount(false, callback); },
                                              kRpcRetryDelay);
          } else {
            LOG(kError) << "UpdateAccount - " << HexSubstr(fob_.identity)
                        << " - Could not set space offered to " << BytesToBinarySiUnits(capacity)
                        << " in account " << HexSubstr(account_name_);
          }

          reschedule_and_respond(result);
        });
  } else {
    // no change, so nothing to do
    reschedule_and_respond(true);
  }
}

void Node::GetSyncData(const protobuf::sync::GetSyncDataRequest& /*request*/,
                       protobuf::sync::GetSyncDataResponse *response) {
  response->set_result(false);

  if (!chunk_info_handler_ || !account_handler_) {
    LOG(kError) << "GetSyncData - " << HexSubstr(fob_.identity) << " - Handlers not initialised.";
    return;
  }

  std::string data(chunk_info_handler_->SerialiseChunkInfo());
  if (!data.empty()) {
    auto sync_data_cih = response->add_sync_data();
    sync_data_cih->set_key("CIH");
    sync_data_cih->set_value(data);
  }

  data = account_handler_->SerialiseAccounts();
  if (!data.empty()) {
    auto sync_data_ah = response->add_sync_data();
    sync_data_ah->set_key("AH");
    sync_data_ah->set_value(data);
  }

  response->set_result(true);
}

void Node::GetProperties(const protobuf::admin::GetPropertiesRequest &request,
                         protobuf::admin::GetPropertiesResponse *response) {
  response->set_result(false);

  std::string filter;
  if (request.has_filter())
    filter = request.filter();

  auto add_property = [response] (const std::string &key, const std::string &value) {
    auto property = response->add_properties();
    property->set_key(key);
    property->set_value(value);
  };

  add_property("node.started", state_ == NodeState::kStarted ? "true" : "false");
  add_property("node.start_time", boost::lexical_cast<std::string>(TimeToEpochTicks(start_time_)));
  // TODO(Steve) add node health
  if (chunk_store_) {
    add_property("store.size", boost::lexical_cast<std::string>(chunk_store_->Size()));
    add_property("store.capacity", boost::lexical_cast<std::string>(chunk_store_->Capacity()));
    add_property("store.count", boost::lexical_cast<std::string>(chunk_store_->Count()));
    add_property("store.cache_size", boost::lexical_cast<std::string>(chunk_store_->CacheSize()));
    add_property("store.cache_capacity",
                 boost::lexical_cast<std::string>(chunk_store_->CacheCapacity()));
    add_property("store.cache_count", boost::lexical_cast<std::string>(chunk_store_->CacheCount()));
  }
  // TODO(Steve) add CIH stats
  // TODO(Steve) add AH stats
  response->set_result(true);
}

void Node::Execute(const protobuf::admin::ExecuteRequest &request,
                   protobuf::admin::ExecuteResponse *response) {
  response->set_result(false);

  if (!fob_.identity.IsInitialised())
    return;

  try {
    const NonEmptyString command(request.command());
    std::string arguments;
    if (request.has_arguments())
      arguments = request.arguments();
    std::string output;

    if (command.string() == "set store.capacity") {
      auto capacity = boost::lexical_cast<std::uintmax_t>(arguments);
      if (chunk_store_) {
        chunk_store_->SetCapacity(capacity);
        output = boost::lexical_cast<std::string>(chunk_store_->Capacity());
        LOG(kInfo) << "Execute - " << HexSubstr(fob_.identity) << " - New chunk store capacity: "
                  << output;
      }
    } else if (command.string() == "set keys") {
      auto new_fob = priv::utils::ParseFob(NonEmptyString(arguments));
      boost::filesystem::path chunk_path_copy = chunk_path();
      int result = Stop();
      if (result != kSuccess) {
        LOG(kError) << "Execute - " << HexSubstr(fob_.identity)
                    << " - Could not stop to set keys.";
        return;
      }
      fob_ = new_fob;
      result = Start(chunk_path_copy);
      if (result != kSuccess) {
        LOG(kError) << "Execute - " << HexSubstr(fob_.identity)
                    << " - Could not restart with new keys.";
        return;
      }
    } else if (command.string() == "ping") {
      output = "pong";
    } else {
      LOG(kError) << "Execute - " << HexSubstr(fob_.identity) << " - Unknown command '"
                  << command.string() << "'.";
      return;
    }

    if (!output.empty())
      response->set_output(output);
    response->set_result(true);
  } catch(const std::exception &ex) {
    LOG(kError) << "Execute - " << HexSubstr(fob_.identity) << " - Exception: " << ex.what();
  }
}

}  // namespace vault

}  // namespace maidsafe
