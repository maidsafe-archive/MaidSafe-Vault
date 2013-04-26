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

#include "maidsafe/vault/maid_account_holder/maid_account_helpers.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"
#include "maidsafe/vault/sync.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<>
int32_t CalculateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&) {
  return 0;
}

template<>
int32_t CalculateCost<passport::PublicMaid>(const passport::PublicMaid&) {
  return 0;
}

template<>
int32_t CalculateCost<passport::PublicPmid>(const passport::PublicPmid&) {
  return 0;
}

}  // namespace detail

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMaidAccountHolder;
}

}  // unnamed namespace



const int MaidAccountHolderService::kPutRepliesSuccessesRequired_(3);
const int MaidAccountHolderService::kDefaultPaymentFactor_(4);

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

void MaidAccountHolderService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                                    const routing::ReplyFunctor& reply_functor) {
  nfs::GenericMessage::Action action(generic_message.action());
  switch (action) {
    case nfs::GenericMessage::Action::kRegisterPmid:
      return HandleRegisterPmid(generic_message, reply_functor);
    case nfs::GenericMessage::Action::kSynchronise:
      return HandlePeriodicSync(generic_message);
    case nfs::GenericMessage::Action::kAccountTransfer:
      return HandleAccountTransfer(generic_message);
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}


// =============== Put/Delete data =================================================================

void MaidAccountHolderService::ValidateSender(const nfs::DataMessage& data_message) const {
  if (!routing_.IsConnectedClient(data_message.source().node_id))
    ThrowError(VaultErrors::permission_denied);

  if (!FromClientMaid(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}


// =============== Pmid registration ===============================================================

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
    std::shared_ptr<PmidRegistrationOp> /*pmid_registration_op*/) {
  return true;
}


// =============== Periodic sync ===================================================================

void MaidAccountHolderService::PeriodicSync(const MaidName& account_name) {
  protobuf::SyncInfo sync_info;
  sync_info.set_maid_account(maid_account_handler_.GetSerialisedAccount(account_name)->string());
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    auto handled_requests(accumulator_.Serialise(account_name));
    sync_info.set_accumulator_entries(handled_requests->string());
  }
  protobuf::Sync sync_pb_message;
  //sync_pb_message.set_action(static_cast<int32_t>(Sync::Action::kSyncInfo));
  sync_pb_message.set_sync_message(sync_info.SerializeAsString());
  assert(sync_pb_message.IsInitialized() && "Uninitialised sync message");
  std::shared_ptr<SharedResponse> shared_response(std::make_shared<SharedResponse>());
  auto callback = [=](std::string response) {
      this->HandlePeriodicSyncCallback(response, account_name, shared_response);
    };
  nfs_.PostSyncDataGroup(account_name,
                         NonEmptyString(sync_pb_message.SerializeAsString()),
                         callback);
}

void MaidAccountHolderService::HandlePeriodicSyncCallback(
    const std::string &response,
    const MaidName& /*account_name*/,
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
    //if (has_file_requests) {
    //  HandleFileRequest(requester_node_id, account_name, sync_info_response.file_hash_requests(),
    //                    shared_response);
    //}
    //CheckAndDeleteAccount(account_name, shared_response);
  } catch(const std::exception& ex) {
    LOG(kError) << "Exception thrown while processing reply : " << ex.what();
  }
}

void MaidAccountHolderService::HandlePeriodicSync(const nfs::GenericMessage& generic_message) {
  NodeId source_id(generic_message.source().node_id);
  if (!routing_.IsConnectedVault(source_id))
    return;

  protobuf::Sync sync_message;
  if (!sync_message.ParseFromString(generic_message.content().string())) {
    LOG(kError) << "Error parsing kSynchronise message.";
    return;
  }
  //try {
  //  Sync::Action sync_action = static_cast<Sync::Action>(sync_message.action());
  //  switch (sync_action) {
  //    case Sync::Action::kSyncInfo:
  //      HandleReceivedSyncInfo(NonEmptyString(sync_message.sync_message()), reply_functor);
  //      break;
  //    case Sync::Action::kSyncArchiveFiles:
  //      break;
  //    default:
  //      LOG(kError) << "Unhandled kSynchronise action type";
  //  }
  //} catch (const std::exception& ex) {
  //  LOG(kError) << "Caught exception on handling sync message: " << ex.what();
  //}
}

void MaidAccountHolderService::HandleReceivedSyncInfo(
    const NonEmptyString &/*serialised_sync_info*/,
    const routing::ReplyFunctor &/*reply_functor*/) {
//  MaidAccount maid_account(serialised_sync_info);
//  return WriteFile(kRootDir_ / maid_account.maid_name().data.string(),
//                   serialised_account.string());
  return;
}


// =============== Account transfer ================================================================

void MaidAccountHolderService::TransferAccount(const MaidName& account_name,
                                               const NodeId& new_node) {
  protobuf::MaidAccount maid_account;
  maid_account.set_maid_name(account_name->string());
  maid_account.set_serialised_account_details(
      maid_account_handler_.GetSerialisedAccount(account_name)->string());
  nfs_.PostAccountTransfer(new_node, NonEmptyString(maid_account.SerializeAsString()));
}

void MaidAccountHolderService::HandleAccountTransfer(const nfs::GenericMessage& generic_message) {
  protobuf::MaidAccount maid_account;
  NodeId source_id(generic_message.source().node_id);
  if (!maid_account.ParseFromString(generic_message.content().string()))
    return;

  MaidName account_name(Identity(maid_account.maid_name()));
  bool finished_all_transfers(
      maid_account_handler_.ApplyAccountTransfer(account_name, source_id,
          MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details()))));
  if (finished_all_transfers)
    UpdatePmidTotals(account_name);
}


// =============== PMID totals =====================================================================

void MaidAccountHolderService::UpdatePmidTotals(const MaidName& /*account_name*/) {
}

void MaidAccountHolderService::UpdatePmidTotalsCallback(
    const std::string& /*response*/,
    const MaidName& /*account_name*/,
    std::shared_ptr<SharedResponse> /*shared_response*/) {
}

void MaidAccountHolderService::HandleChurnEvent(routing::MatrixChange matrix_change) {
  auto account_names(maid_account_handler_.GetAccountNames());
  auto itr(std::begin(account_names));
  while (itr != std::end(account_names)) {
    auto check_holders_result(CheckHolders(matrix_change, routing_.kNodeId(),
                                           NodeId((*itr)->string())));
    // Delete accounts for which this node is no longer responsible.
    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      maid_account_handler_.DeleteAccount(*itr);
      itr = account_names.erase(itr);
      continue;
    }

    // Replace old_node(s) in sync object and send AccountTransfer to new node(s).
    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
      maid_account_handler_.ReplaceNodeInSyncList(*itr, check_holders_result.old_holders[i],
                                                  check_holders_result.new_holders[i]);
      TransferAccount(*itr, check_holders_result.new_holders[i]);
    }

    ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe
