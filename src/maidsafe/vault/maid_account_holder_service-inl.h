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
  try {
    nfs::ReturnCode return_code(MakeError(CommonErrors::success));
    auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
    if (accumulator_.CheckHandled(request_id, return_code))
      return reply_functor(return_code.Serialise()->string());

    ValidateDataMessage(data_message);
    check with MM if data is unique
    AdjustAccount<Data>(data_message, is_payable<Data>());
    SendDataMessage<Data>(data_message);

    assert(return_code.value() == static_cast<int>(CommonErrors::success));
    accumulator_.SetHandled(request_id, return_code);
    reply_functor(return_code.Serialise()->string());
  }
  catch(const std::system_error& error) {
    reply_functor(nfs::ReturnCode(error).Serialise()->string());
  }
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
