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

#ifndef MAIDSAFE_VAULT_ACCOUNT_HOLDER_INL_H_
#define MAIDSAFE_VAULT_ACCOUNT_HOLDER_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  if (accumulator_.CheckHandled(request_id, return_code))
    return reply_functor(return_code.Serialise()->string());

  if (data_message.action() == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.action() == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    return_code = nfs::ReturnCode(MakeError(VaultErrors::operation_not_supported));
    accumulator_.SetHandled(request_id, return_code);
    reply_functor(return_code.Serialise()->string());
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  try {
    ValidateDataMessage(data_message);
    check with MM if data is unique.  If MM says already stored, check if we stored it.  If so success, else failure.
    AdjustAccount<Data>(data_message, is_payable<Data>());
    SendDataMessage<Data>(data_message);
  }
  catch(const std::system_error& error) {
    LOG(kWarning) << error.what();
    return_code = nfs::ReturnCode(error);
  }
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  accumulator_.SetHandled(request_id, return_code);
  reply_functor(return_code.Serialise()->string());
}

template<typename Data>
void MaidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateDataMessage(data_message);
    AdjustAccount<Data>(data_message, is_payable<Data>());
    SendDataMessage<Data>(data_message);
  }
  catch(const std::system_error& error) {
    LOG(kWarning) << error.what();
    // Always return succeess for Deletes
  }
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  accumulator_.SetHandled(request_id, return_code);
  reply_functor(return_code.Serialise()->string());
}

template<typename Data>
void MaidAccountHolderService::AdjustAccount(const nfs::DataMessage& data_message, std::true_type) {
  if (data_message.action() == nfs::DataMessage::Action::kPut) {
    maid_account_handler_.PutData<Data>(
        detail::GetSourceMaidName(data_message),
        Data::name_type(data_message.data().name),
        static_cast<int32_t>(data_message.data().content.string().size()),
        1);
  } else {
    assert(data_message.action() == nfs::DataMessage::Action::kDelete);
    maid_account_handler_.DeleteData<Data>(detail::GetSourceMaidName(data_message),
                                           Data::name_type(data_message.data().name));
  }
}

template<typename Data>
void MaidAccountHolderService::SendDataMessage(const nfs::DataMessage& data_message) {
  if (data_message.action() == nfs::DataMessage::Action::kPut) {
    nfs_.Put<Data>(
        data_message,
        [&routing_, &nfs_](nfs::DataMessage data_msg) {
            detail::RetryOnPutOrDeleteError<MaidAccountHolderNfs, Data>(routing_, nfs_, data_msg);
        });
  } else {
    assert(data_message.action() == nfs::DataMessage::Action::kDelete);
    nfs_.Delete<Data>(
        data_message,
        [&routing_, &nfs_](nfs::DataMessage data_msg) {
            detail::RetryOnPutOrDeleteError<MaidAccountHolderNfs, Data>(routing_, nfs_, data_msg);
        });
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_HOLDER_INL_H_
