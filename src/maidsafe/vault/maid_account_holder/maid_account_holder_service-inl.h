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
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data>
int32_t CalculateCost(const Data& data) {
  static_assert(!std::is_same<Data, passport::PublicAnmaid>::value, "Cost of Anmaid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicMaid>::value, "Cost of Maid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicPmid>::value, "Cost of Pmid should be 0.");
  return static_cast<int32_t>(MaidAccountHolderService::DefaultPaymentFactor() *
                              data.content.string().size());
}

template<>
int32_t CalculateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&);

template<>
int32_t CalculateCost<passport::PublicMaid>(const passport::PublicMaid&);

template<>
int32_t CalculateCost<passport::PublicPmid>(const passport::PublicPmid&);

}  // namespace detail

template<>
void MaidAccountHolderService::PmidRegistrationOp::SetPublicFob<passport::PublicMaid>(
    std::unique_ptr<passport::PublicMaid>&& pub_maid);

template<>
void MaidAccountHolderService::PmidRegistrationOp::SetPublicFob<passport::PublicPmid>(
    std::unique_ptr<passport::PublicPmid>&& pub_pmid);

template<typename Data>
void MaidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
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
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(data_message, reply.error());
    reply_functor(reply.Serialise()->string());
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  maidsafe_error return_code(CommonErrors::success);
  try {
    ValidateSender(data_message);
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    MaidName account_name(detail::GetSourceMaidName(data_message));
    auto data_name(GetDataName<Data>(data_message));
    auto put_op(std::make_shared<nfs::OperationOp>(
        kPutRepliesSuccessesRequired_,
        [this, account_name, data_name, reply_functor](nfs::Reply overall_result) {
            this->HandlePutResult<Data>(overall_result, account_name, data_name, reply_functor,
                                  overall_result.IsSuccess(), is_unique_on_network<Data>());
        }));
    // TODO(Fraser#5#): 2013-02-13 - Have PutToAccount return percentage or amount remaining so
    // if it falls below a threshold, we can trigger getting updated account info from the PAHs
    // (not too frequently), & alert the client by returning an "error" via client_reply_functor.
    //auto size(detail::CalculateCost(data_message.data()));
    //PutToAccount<Data>(account_name, data_name, size, is_payable<Data>());
    //on_scope_exit strong_guarantee([this, account_name, data_name] {
    //    try {
    //      this->DeleteFromAccount<Data>(account_name, data_name, is_payable<Data>());
    //    }
    //    catch(const std::exception& e) {
    //      LOG(kError) << "Failed to delete from account: " << e.what();
    //    }
    //});
    nfs_.Put(data,
             data_message.data_holder(),
             [put_op](std::string serialised_reply) {
                 nfs::HandleOperationReply(put_op, serialised_reply);
             });
    SendEarlySuccessReply<Data>(data_message, reply_functor,
                                false, is_unique_on_network<Data>());
    // TODO(dirvine) check above false value is OK
    //strong_guarantee.Release();
    return;
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    return_code = error;
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    return_code = MakeError(CommonErrors::unknown);
  }
  try {
    detail::SendReply(data_message, return_code, reply_functor);
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(data_message, return_code);
  }
  catch(...) {
    LOG(kWarning) << "Exception while forming reply.";
  }
}

template<typename Data>
void MaidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateSender(data_message);
    auto data_name(GetDataName<Data>(data_message));
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
  auto success_code(MakeError(CommonErrors::success));
  detail::SendReply(data_message, success_code, reply_functor);
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(data_message, success_code);
}

template<typename Data>
typename Data::name_type MaidAccountHolderService::GetDataName(
    const nfs::DataMessage& data_message) const {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return typename Data::name_type(crypto::Hash<crypto::SHA512>(data_message.data().name));
}

template<typename Data>
void MaidAccountHolderService::SendEarlySuccessReply(const nfs::DataMessage& data_message,
                                                     const routing::ReplyFunctor& reply_functor,
                                                     bool low_space,
                                                     NonUniqueDataType) {
  nfs::Reply reply(CommonErrors::success);
  if (low_space)
    reply = VaultErrors::low_space;
  reply_functor(reply.Serialise()->string());
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(data_message, reply.error());
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const MaidName& /*account_name*/,
                                               const typename Data::name_type& /*data_name*/,
                                               routing::ReplyFunctor client_reply_functor,
                                               bool low_space,
                                               UniqueDataType) {
  if (overall_result.IsSuccess()) {
    nfs::Reply reply(CommonErrors::success);
    if (low_space)
      reply = VaultErrors::low_space;
    client_reply_functor(reply.Serialise()->string());
  } else {
    client_reply_functor(overall_result.Serialise()->string());
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const MaidName& /*account_name*/,
                                               const typename Data::name_type& /*data_name*/,
                                               routing::ReplyFunctor /*client_reply_functor*/,
                                               bool /*low_space*/,
                                               NonUniqueDataType) {
  try {
    if (overall_result.IsSuccess()) {
      protobuf::Cost cost;
      cost.ParseFromString(overall_result.data().string());
      //AdjustAccount<Data>(account_name, data_name, cost.value(), is_payable<Data>());
    }
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
  }
}

template<typename PublicFobType>
void MaidAccountHolderService::ValidateRegisterPmid(
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
    FinaliseRegisterPmid(pmid_registration_op);
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_INL_H_
