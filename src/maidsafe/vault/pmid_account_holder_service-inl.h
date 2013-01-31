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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_

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
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  if (accumulator_.CheckHandled(request_id, return_code))
    return reply_functor(return_code.Serialise()->string());

  ValidateDataMessage(data_message);

  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    return_code = nfs::ReturnCode(MakeError(VaultErrors::operation_not_supported));
    accumulator_.SetHandled(request_id, return_code);
    reply_functor(return_code.Serialise()->string());
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  try {
    std::future<void> future(pmid_account_handler_.PutData<Data>(
                                 data_message.target_id(),
                                 data_message.data().name,
                                 data_message.data().content.string.size()));
    try {
      future.get();
      SendDataMessage<Data>(data_message);
    }
    catch(const std::exception& e) {
      LOG(kError) << "Operation with the account threw: " << e.what();
      return_code = nfs::ReturnCode(e);
    }
  }
  catch(const std::system_error& error) {
    LOG(kError) << "Failure deleting data from account: " << error.what();
    return_code = nfs::ReturnCode(error);
  }
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  accumulator_.SetHandled(request_id, return_code);
  reply_functor(return_code.Serialise()->string());
}

template<typename Data>
void PmidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    pmid_account_handler_.DeleteData<Data>(data_message.target_id(), data_message.data().name);
    SendDataMessage<Data>(data_message);
  }
  catch(const std::system_error& error) {
    LOG(kError) << "Failure deleting data from account: " << error.what();
    // Always return succeess for Deletes
  }
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  nfs::ReturnCode return_code(MakeError(CommonErrors::success));
  accumulator_.SetHandled(request_id, return_code);
  reply_functor(return_code.Serialise()->string());
}

template<typename Data>
void PmidAccountHolderService::SendDataMessage(const nfs::DataMessage& data_message) {
  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    nfs_.Put<Data>(data_message,
                   [this] (nfs::DataMessage data_msg) {
                     pmid_account_handler_.DeleteData<Data>(data_msg.target_id(),
                                                            data_msg.data().name);
                   });
  } else {
    assert(data_message.data().action == nfs::DataMessage::Action::kDelete);
    nfs_.Delete<Data>(data_message,
                      [this] (nfs::DataMessage data_msg) {
                        detail::RetryOnPutOrDeleteError<PmidAccountHolderNfs, Data>(routing_,
                                                                                    nfs_,
                                                                                    data_msg);
                      });
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
