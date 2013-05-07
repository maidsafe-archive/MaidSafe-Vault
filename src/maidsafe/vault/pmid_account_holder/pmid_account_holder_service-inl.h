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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void PmidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
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
void PmidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateSender(data_message);
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    if (detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired_)) {
      pmid_account_handler_.Put<Data>(data_message.data_holder(), data.name(),
          static_cast<int32_t>(data_message.data().content.string().size()));
      nfs_.Put(data_message.data_holder(), data, nullptr);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(data_message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kPutRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kPutRequestsRequired_);
  }
}

template<typename Data>
void PmidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateSender(data_message);
    typename Data::name_type data_name(data_message.data().name);
    if (detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired_)) {
      pmid_account_handler_.Delete<Data>(data_message.data_holder(), data_name);
      nfs_.Delete<Data>(data_message.data_holder(), data_name, nullptr);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(data_message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired_);
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
