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

#include "maidsafe/vault/maid_account_holder/maid_account_holder_service.h"

#include <string>

#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"
#include "maidsafe/vault/sync.h"


namespace maidsafe {

namespace vault {

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMaidAccountHolder;
}

}  // unnamed namespace

const int MaidAccountHolderService::kPutRepliesSuccessesRequired_(3);
const int MaidAccountHolderService::kDefaultPaymentFactor_(4);

MaidAccountHolderService::SharedResponse::SharedResponse()
    : mutex(),
      count(0),
      this_node_in_group(false) {}

MaidAccountHolderService::MaidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   Db& db)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_mutex_(),
      accumulator_(),
      maid_account_handler_(db, routing.kNodeId()),
      nfs_(routing, pmid) {}

void MaidAccountHolderService::ValidateSender(const nfs::DataMessage& data_message) const {
  if (!routing_.IsConnectedClient(data_message.source().node_id))
    ThrowError(VaultErrors::permission_denied);

  if (!FromClientMaid(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MaidAccountHolderService::HandleChurnEvent(const NodeId& old_node, const NodeId& new_node) {
  auto account_names(maid_account_handler_.GetAccountNames());
  auto itr(std::begin(account_names));
  while (itr != std::end(account_names)) {
    // delete accounts we are no longer responsible for
    if (routing_.IsNodeIdInGroupRange(NodeId((*itr)->string())) !=
        routing::GroupRangeStatus::kInRange) {
      maid_account_handler_.DeleteAccount(*itr);
      itr = account_names.erase(itr);
      continue;
    }

    // replace old_node in sync object and send AccountTransfer to new node.
    if (routing_.IsNodeIdInGroupRange(NodeId((*itr)->string()), old_node) ==
        routing::GroupRangeStatus::kInRange) {
      maid_account_handler_.ReplaceNodeInSyncList(*itr, old_node, new_node);
      TransferAccount(*itr, new_node);
    }
    ++itr;
  }
}

void MaidAccountHolderService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                                    const routing::ReplyFunctor& reply_functor) {
  nfs::GenericMessage::Action action(generic_message.action());
  switch (action) {
    case nfs::GenericMessage::Action::kRegisterPmid:
      return HandleRegisterPmid(generic_message, reply_functor);
    case nfs::GenericMessage::Action::kConnect:
      break;
    case nfs::GenericMessage::Action::kGetPmidHealth:
      break;
    case nfs::GenericMessage::Action::kNodeDown:
      break;
    case nfs::GenericMessage::Action::kSynchronise:
      return HandleSyncMessage(generic_message, reply_functor);
    case nfs::GenericMessage::Action::kAccountTransfer:
      return HandleAccountTransfer(generic_message, reply_functor);
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

void MaidAccountHolderService::HandleRegisterPmid(const nfs::GenericMessage& generic_message,
                                                  const routing::ReplyFunctor& reply_functor) {
  NodeId source_id(generic_message.source().node_id);
  if (!routing_.IsConnectedClient(source_id))
    return reply_functor(nfs::Reply(RoutingErrors::not_connected).Serialise()->string());

  // TODO(Fraser#5#): 2013-04-22 - Validate Message signature.  Currently the Message does not have
  //                  a signature applied, and the demuxer doesn't pass the signature down anyway.
  nfs::PmidRegistration pmid_registration(nfs::PmidRegistration::serialised_type(NonEmptyString(
      generic_message.content().string())));
  if (pmid_registration.maid_name()->string() != source_id.string())
    ThrowError(VaultErrors::permission_denied);

  auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(pmid_registration, reply_functor));

  public_key_getter_.GetKey<passport::PublicMaid>(
      pmid_registration.maid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidateRegisterPmid<passport::PublicMaid>(reply, pmid_registration.maid_name(),
                                                     pmid_registration_op);
      });
  public_key_getter_.GetKey<passport::PublicPmid>(
      pmid_registration.pmid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidateRegisterPmid<passport::PublicPmid>(reply, pmid_registration.pmid_name(),
                                                     pmid_registration_op);
      });
}

void MaidAccountHolderService::FinaliseRegisterPmid(
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
  assert(pmid_registration_op->count == 2);

  auto send_reply([&](const maidsafe_error& error)->void {
      nfs::Reply reply(error);
      pmid_registration_op->reply_functor(reply.Serialise()->string());
  });

  if (!pmid_registration_op->public_maid || !pmid_registration_op->public_pmid) {
    LOG(kWarning) << "Failed to retrieve one or both of MAID and PMID";
    return send_reply(maidsafe_error(VaultErrors::permission_denied));
  }

  if (!pmid_registration_op->pmid_registration.Validate(*pmid_registration_op->public_maid,
                                                        *pmid_registration_op->public_pmid)) {
    LOG(kWarning) << "Failed to validate PmidRegistration";
    return send_reply(maidsafe_error(VaultErrors::permission_denied));
  }

  if (!DoRegisterPmid(pmid_registration_op)) {
    LOG(kWarning) << "Failed to register new PMID";
    return send_reply(maidsafe_error(VaultErrors::failed_to_handle_request));
  }

  send_reply(maidsafe_error(CommonErrors::success));
}

bool MaidAccountHolderService::DoRegisterPmid(
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {

}

void MaidAccountHolderService::HandleSyncMessage(const nfs::GenericMessage& generic_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  NodeId source_id(generic_message.source().node_id);
  if (!routing_.IsConnectedVault(source_id))
    return reply_functor(nfs::Reply(RoutingErrors::not_connected).Serialise()->string());

  protobuf::Sync sync_message;
  if (!sync_message.ParseFromString(generic_message.content().string()) ||
          !sync_message.IsInitialized()) {
    LOG(kError) << "Error parsing kSynchronise message.";
//    reply_functor();  // FIXME  Is this needed 
    return;
  }
  try {
    Sync::Action sync_action = static_cast<Sync::Action>(sync_message.action());
    switch (sync_action) {
      case Sync::Action::kSyncInfo:
        HandleReceivedSyncInfo(NonEmptyString(sync_message.sync_message()), reply_functor);
        break;
      case Sync::Action::kSyncArchiveFiles:
        break;
      default:
        LOG(kError) << "Unhandled kSynchronise action type";
    }
  } catch (const std::exception& ex) {
    LOG(kError) << "Caught exception on handling sync message: " << ex.what();
  }
}

// bool MaidAccountHolderService::HandleNewComer(const passport::/*PublicMaid*/PublicPmid& p_maid) {
//   std::promise<bool> result_promise;
//   std::future<bool> result_future = result_promise.get_future();
//   auto get_key_future([this, p_maid, &result_promise] (
//       std::future<maidsafe::passport::PublicPmid> key_future) {
//     try {
//       maidsafe::passport::PublicPmid p_pmid = key_future.get();
//       result_promise.set_value(OnKeyFetched(p_maid, p_pmid));
//     }
//     catch(const std::exception& ex) {
//       LOG(kError) << "Failed to get key for " << HexSubstr(p_maid.name().data.string())
//                   << " : " << ex.what();
//       result_promise.set_value(false);
//     }
//   });
//   public_key_getter_.HandleGetKey<maidsafe::passport::PublicPmid>(p_maid.name(), get_key_future);
//   return result_future.get();
// }
//
// bool MaidAccountHolderService::OnKeyFetched(const passport::/*PublicMaid*/PublicPmid& p_maid,
//                                      const passport::PublicPmid& p_pmid) {
//   if (!asymm::CheckSignature(asymm::PlainText(asymm::EncodeKey(p_pmid.public_key())),
//                              p_pmid.validation_token(), p_maid.public_key())) {
//     LOG(kError) << "Fetched pmid for " << HexSubstr(p_maid.name().data.string())
//                 << " contains invalid token";
//     return false;
//   }
//
//   maidsafe::nfs::MaidAccount maid_account(p_maid.name().data);
//   maid_account.PushPmidTotal(nfs::PmidTotals(nfs::PmidRegistration(p_maid.name().data,
//                                                                   p_pmid.name().data,
//                                                                   false,
//                                                                   p_maid.validation_token(),
//                                                                   p_pmid.validation_token()),
//                                             nfs::PmidSize(p_pmid.name().data)));
//   return WriteFile(kRootDir_ / maid_account.maid_id().string(),
//                    maid_account.Serialise().string());
// }

// bool MaidAccountHolderService::HandleNewComer(const nfs::PmidRegistration& pmid_registration) {
//   Identity maid_id(pmid_registration.maid_id());
//   MaidAccount maid_account(maid_id);
//   maid_account.PushPmidTotal(PmidTotals(pmid_registration, PmidRecord(maid_id)));
//   return WriteFile(kRootDir_ / maid_id.string(), maid_account.Serialise().string());
// }

void MaidAccountHolderService::SendSyncData(const MaidName& account_name) {
  protobuf::SyncInfo sync_info;
  sync_info.set_maid_account(
      maid_account_handler_.GetSerialisedAccountSyncInfo(account_name)->string());
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    auto handled_requests(accumulator_.Serialise(account_name));
    sync_info.set_accumulator_entries(handled_requests->string());
  }
  protobuf::Sync sync_pb_message;
  sync_pb_message.set_action(static_cast<int32_t>(Sync::Action::kSyncInfo));
  sync_pb_message.set_sync_message(sync_info.SerializeAsString());
  assert(sync_pb_message.IsInitialized() && "Uninitialised sync message");
  std::shared_ptr<SharedResponse> shared_response(std::make_shared<SharedResponse>());
  auto callback = [=](std::string response) {
      this->HandleSendSyncDataCallback(response, account_name, shared_response);
    };
  nfs_.PostSyncDataGroup(account_name,
                         NonEmptyString(sync_pb_message.SerializeAsString()),
                         callback);
}

void MaidAccountHolderService::HandleSendSyncDataCallback(
    const std::string &response,
    const MaidName& account_name,
    std::shared_ptr<SharedResponse> shared_response) {
  try {
    protobuf::SyncInfoResponse sync_info_response;
    nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(response))));
    if (!reply.IsSuccess() || !sync_info_response.ParseFromString(reply.data().string())) {
      LOG(kError) << "Failed to parse reply";
      return;
    }
    NodeId requester_node_id(sync_info_response.node_id());
    bool has_file_requests(sync_info_response.has_file_hash_requests());
    {
      std::lock_guard<std::mutex> lock(shared_response->mutex);
      if (requester_node_id == routing_.kNodeId())
        shared_response->this_node_in_group = true;
      if (!has_file_requests)
        ++shared_response->count;  //  Done with this node
    }
    if (has_file_requests) {
      HandleFileRequest(requester_node_id, account_name, sync_info_response.file_hash_requests(),
                        shared_response);
    }
    CheckAndDeleteAccount(account_name, shared_response);
  } catch(const std::exception& ex) {
    LOG(kError) << "Exception thrown while processing reply : " << ex.what();
  }
}

void MaidAccountHolderService::HandleAccountTransfer(const nfs::GenericMessage& generic_message,
                                                     const routing::ReplyFunctor& reply_functor) {
  protobuf::MaidAccount maid_account;
  if (maid_account.ParseFromString(generic_message.content().string())) {
    MaidName account_name(Identity(maid_account.maid_name()));
    maid_account_handler_.ApplyAccountTransfer(account_name,
        MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details())));
  }
}

void MaidAccountHolderService::HandleReceivedSyncInfo(
    const NonEmptyString &/*serialised_sync_info*/,
    const routing::ReplyFunctor &/*reply_functor*/) {
//  MaidAccount maid_account(serialised_sync_info);
//  return WriteFile(kRootDir_ / maid_account.maid_name().data.string(),
//                   serialised_account.string());
  return;
}

}  // namespace vault

}  // namespace maidsafe
