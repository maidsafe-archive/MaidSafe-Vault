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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_INL_H_

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
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename T>
struct can_create_account : public std::false_type {};

template<>
struct can_create_account<passport::PublicAnmaid> : public std::true_type {};

template<>
struct can_create_account<passport::PublicMaid> : public std::true_type {};

template<typename Data>
int32_t EstimateCost(const Data& data) {
  static_assert(!std::is_same<Data, passport::PublicAnmaid>::value, "Cost of Anmaid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicMaid>::value, "Cost of Maid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicPmid>::value, "Cost of Pmid should be 0.");
  return static_cast<int32_t>(MaidAccountHolderService::DefaultPaymentFactor() *
                              data.content.string().size());
}

template<>
int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&);

template<>
int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&);

template<>
int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&);

MaidName GetMaidAccountName(const nfs::DataMessage& data_message);

template<typename Data>
typename Data::name_type GetDataName(const nfs::DataMessage& data_message) {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return typename Data::name_type(crypto::Hash<crypto::SHA512>(data_message.data().name));
}

template<typename Data, nfs::MessageAction action>
MaidAccountUnresolvedEntry CreateUnresolvedEntry(const nfs::DataMessage& data_message,
                                                 int32_t cost,
                                                 const NodeId& this_id) {
  static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return MaidAccountUnresolvedEntry(
      std::make_pair(DataNameVariant(GetDataName<Data>(data_message)), action), cost, this_id);
}

}  // namespace detail


template<typename Data>
void MaidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  ValidateSender(data_message);
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(data_message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported, data_message.Serialise().data);
    SendReplyAndAddToAccumulator(data_message, reply_functor, reply);
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  maidsafe_error return_code(CommonErrors::success);
  try {
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    auto account_name(detail::GetMaidAccountName(data_message));
    auto estimated_cost(detail::EstimateCost(data_message.data()));
    maid_account_handler_.CreateAccount<Data>(account_name, detail::can_create_account<Data>());
    auto account_status(maid_account_handler_.AllowPut(account_name, estimated_cost));

    if (account_status == MaidAccount::Status::kNoSpace)
      ThrowError(VaultErrors::not_enough_space);
    bool low_space(account_status == MaidAccount::Status::kLowSpace);

    auto put_op(std::make_shared<nfs::OperationOp>(
        kPutRepliesSuccessesRequired_,
        [this, data_message, reply_functor, low_space](nfs::Reply overall_result) {
            this->HandlePutResult<Data>(overall_result, data_message, reply_functor,
                                        low_space, is_unique_on_network<Data>());
        }));

    if (low_space)
      UpdatePmidTotals(account_name);

    nfs_.Put(data,
             data_message.data_holder(),
             [put_op](std::string serialised_reply) {
                 nfs::HandleOperationReply(put_op, serialised_reply);
             });
    return SendEarlySuccessReply<Data>(data_message, reply_functor, low_space,
                                       is_unique_on_network<Data>());
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    return_code = error;
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    return_code = MakeError(CommonErrors::unknown);
  }
  nfs::Reply reply(return_code, data_message.Serialise().data);
  SendReplyAndAddToAccumulator(data_message, reply_functor, reply);
}

template<typename Data>
void MaidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    auto data_name(detail::GetDataName<Data>(data_message));
    //DeleteFromAccount<Data>(detail::GetSourceMaidName(data_message), data_name, is_payable<Data>());
    nfs_.Delete<Data>(data_name, [](std::string /*serialised_reply*/) {});
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    // Always return success for Deletes
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    // Always return success for Deletes
  }
  SendReplyAndAddToAccumulator(data_message, reply_functor, nfs::Reply(CommonErrors::success));
}

template<typename Data>
void MaidAccountHolderService::SendEarlySuccessReply(const nfs::DataMessage& data_message,
                                                     const routing::ReplyFunctor& reply_functor,
                                                     bool low_space,
                                                     NonUniqueDataType) {
  nfs::Reply reply(CommonErrors::success);
  if (low_space)
    reply = nfs::Reply(VaultErrors::low_space);
  SendReplyAndAddToAccumulator(data_message, reply_functor, reply);
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::DataMessage& data_message,
                                               routing::ReplyFunctor client_reply_functor,
                                               bool low_space,
                                               UniqueDataType) {
  if (overall_result.IsSuccess()) {
    nfs::Reply reply(CommonErrors::success);
    if (low_space)
      reply = nfs::Reply(VaultErrors::low_space);
    SendReplyAndAddToAccumulator(data_message, client_reply_functor, reply);
    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(
        data_message,
        detail::EstimateCost(data_message.data()));
  } else {
    SendReplyAndAddToAccumulator(data_message, client_reply_functor, overall_result);
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::DataMessage& data_message,
                                               routing::ReplyFunctor /*client_reply_functor*/,
                                               bool /*low_space*/,
                                               NonUniqueDataType) {
  try {
    if (overall_result.IsSuccess()) {
      protobuf::Cost proto_cost;
      proto_cost.ParseFromString(overall_result.data().string());
      AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(data_message,
                                                                      proto_cost.cost());
    }
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
  }
}

template<typename Data, nfs::MessageAction action>
void MaidAccountHolderService::AddLocalUnresolvedEntryThenSync(const nfs::DataMessage& data_message,
                                                               int32_t cost) {
  auto account_name(detail::GetMaidAccountName(data_message));
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(data_message, cost,
                                                                    routing_.kNodeId()));
  maid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
  Sync(account_name);
}

template<typename PublicFobType>
void MaidAccountHolderService::ValidatePmidRegistration(
    const nfs::Reply& reply,
    typename PublicFobType::name_type public_fob_name,
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
  std::unique_ptr<PublicFobType> public_fob;
  try {
    public_fob.reset(new PublicFobType(public_fob_name,
                                       typename PublicFobType::serialised_type(reply.data())));
  }
  catch(const std::exception& e) {
    public_fob.reset();
    LOG(kError) << e.what();
  }
  bool finalise(false);
  {
    std::lock_guard<std::mutex> lock(pmid_registration_op->mutex);
    pmid_registration_op->SetPublicFob(std::move(public_fob));
    finalise = (++pmid_registration_op->count == 2);
  }
  if (finalise)
    FinalisePmidRegistration(pmid_registration_op);
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_INL_H_
