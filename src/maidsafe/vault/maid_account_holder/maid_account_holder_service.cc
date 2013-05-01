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
int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&) {
  return 0;
}

template<>
int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&) {
  return 0;
}

template<>
int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&) {
  return 0;
}

MaidName GetMaidAccountName(const nfs::DataMessage& data_message) {
  return MaidName(Identity(data_message.source().node_id.string()));
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
  ValidateSender(generic_message);
  nfs::GenericMessage::Action action(generic_message.action());
  switch (action) {
    case nfs::GenericMessage::Action::kRegisterPmid:
      return HandlePmidRegistration(generic_message, reply_functor);
    case nfs::GenericMessage::Action::kSynchronise:
      return HandleSync(generic_message);
    case nfs::GenericMessage::Action::kAccountTransfer:
      return HandleAccountTransfer(generic_message);
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

void MaidAccountHolderService::ValidateSender(const nfs::DataMessage& data_message) const {
  if (!routing_.IsConnectedClient(data_message.source().node_id))
    ThrowError(VaultErrors::permission_denied);

  if (!FromClientMaid(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MaidAccountHolderService::ValidateSender(const nfs::GenericMessage& generic_message) const {
  if (generic_message.action() == nfs::GenericMessage::Action::kRegisterPmid) {
    if (!routing_.IsConnectedClient(generic_message.source().node_id))
      ThrowError(VaultErrors::permission_denied);
    if (!FromClientMaid(generic_message) || !ForThisPersona(generic_message))
      ThrowError(CommonErrors::invalid_parameter);
  } else {
    if (!routing_.IsConnectedVault(generic_message.source().node_id))
      ThrowError(VaultErrors::permission_denied);
    if (!FromMaidAccountHolder(generic_message) || !ForThisPersona(generic_message))
      ThrowError(CommonErrors::invalid_parameter);
  }
}


// =============== Put/Delete data =================================================================

void MaidAccountHolderService::SendReplyAndAddToAccumulator(
    const nfs::DataMessage& data_message,
    const routing::ReplyFunctor& reply_functor,
    const nfs::Reply& reply) {
  reply_functor(reply.Serialise()->string());
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(data_message, reply.error());
}


// =============== Pmid registration ===============================================================

void MaidAccountHolderService::HandlePmidRegistration(const nfs::GenericMessage& generic_message,
                                                      const routing::ReplyFunctor& reply_functor) {
  NodeId source_id(generic_message.source().node_id);

  // TODO(Fraser#5#): 2013-04-22 - Validate Message signature.  Currently the Message does not have
  //                  a signature applied, and the demuxer doesn't pass the signature down anyway.
  nfs::PmidRegistration pmid_registration(nfs::PmidRegistration::serialised_type(NonEmptyString(
      generic_message.content().string())));
  if (pmid_registration.maid_name()->string() != source_id.string())
    return reply_functor(nfs::Reply(VaultErrors::permission_denied).Serialise()->string());

  auto pmid_registration_op(std::make_shared<PmidRegistrationOp>(pmid_registration, reply_functor));

  public_key_getter_.GetKey<passport::PublicMaid>(
      pmid_registration.maid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidatePmidRegistration<passport::PublicMaid>(reply, pmid_registration.maid_name(),
                                                         pmid_registration_op);
      });
  public_key_getter_.GetKey<passport::PublicPmid>(
      pmid_registration.pmid_name(),
      [this, pmid_registration_op, &pmid_registration](const nfs::Reply& reply) {
          ValidatePmidRegistration<passport::PublicPmid>(reply, pmid_registration.pmid_name(),
                                                         pmid_registration_op);
      });
}

void MaidAccountHolderService::FinalisePmidRegistration(
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

  try {
    if (!pmid_registration_op->pmid_registration.Validate(*pmid_registration_op->public_maid,
                                                          *pmid_registration_op->public_pmid)) {
      LOG(kWarning) << "Failed to validate PmidRegistration";
      return send_reply(maidsafe_error(VaultErrors::permission_denied));
    }

    if (pmid_registration_op->pmid_registration.unregister()) {
      maid_account_handler_.UnregisterPmid(pmid_registration_op->public_maid->name(),
                                           pmid_registration_op->public_pmid->name());
    } else {
      maid_account_handler_.RegisterPmid(pmid_registration_op->public_maid->name(),
                                         pmid_registration_op->pmid_registration);
    }
    send_reply(maidsafe_error(CommonErrors::success));
    UpdatePmidTotals(pmid_registration_op->public_maid->name());
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "Failed to register new PMID: " << error.what();
    send_reply(error);
  }
  catch(const std::exception& ex) {
    LOG(kWarning) << "Failed to register new PMID: " << ex.what();
    send_reply(maidsafe_error(CommonErrors::unknown));
  }
}


// =============== Sync ============================================================================

void MaidAccountHolderService::Sync(const MaidName& account_name) {
  auto serialised_sync_data(maid_account_handler_.GetSyncData(account_name));
  if (!serialised_sync_data.IsInitialised())  // Nothing to sync
    return;

  protobuf::Sync proto_sync;
  proto_sync.set_account_name(account_name->string());
  proto_sync.set_serialised_unresolved_entries(serialised_sync_data.string());

  nfs_.Sync(account_name, NonEmptyString(proto_sync.SerializeAsString()));
}

void MaidAccountHolderService::HandleSync(const nfs::GenericMessage& generic_message) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(generic_message.content().string())) {
    LOG(kError) << "Error parsing kSynchronise message.";
    return;
  }

  MaidName account_name(Identity(proto_sync.account_name()));
  maid_account_handler_.ApplySyncData(account_name, generic_message.source().node_id,
                                      NonEmptyString(proto_sync.serialised_unresolved_entries()));
}


// =============== Account transfer ================================================================

void MaidAccountHolderService::TransferAccount(const MaidName& account_name,
                                               const NodeId& new_node) {
  protobuf::MaidAccount maid_account;
  maid_account.set_maid_name(account_name->string());
  maid_account.set_serialised_account_details(
      maid_account_handler_.GetSerialisedAccount(account_name)->string());
  nfs_.TransferAccount(new_node, NonEmptyString(maid_account.SerializeAsString()));
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

void MaidAccountHolderService::UpdatePmidTotals(const MaidName& account_name) {
  auto pmid_names(maid_account_handler_.GetPmidNames(account_name));
  for (const auto& pmid_name : pmid_names) {
    auto op_data(std::make_shared<GetPmidTotalsOp>(account_name));
    nfs_.RequestPmidTotals(pmid_name,
                           [this, op_data](std::string serialised_reply) {
                             UpdatePmidTotalsCallback(serialised_reply, op_data);
                           });
  }
}

void MaidAccountHolderService::UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                                        std::shared_ptr<GetPmidTotalsOp> op_data) {
  // 1] Parse reply - don't throw - need to add default-constructed PmidRecord if fails
  // 2] Lock the mutex
  // 3] add result to vector (assert <= 4)
  // 4] if vector size < 4 return
  // 5] else merge results and do maid_account_handler_.UpdatePmidTotals
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
