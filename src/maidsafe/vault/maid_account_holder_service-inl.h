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

#include "maidsafe/nfs/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  auto request_id(std::make_pair(data_message.message_id(), data_message.source().persona));
  if (accumulator_.CheckHandled(request_id, reply))
    return reply_functor(reply.Serialise()->string());

  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported);
    accumulator_.SetHandled(request_id, reply);
    reply_functor(reply.Serialise()->string());
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  nfs::Accumulator<MaidName>::RequestIdentity request_id;
  try {
    ValidateDataMessage(data_message);
    Data data(data_message.data().name, Data::serialised_type(data_message.data().content));
    MaidName account_name(detail::GetSourceMaidName(data_message));
    auto data_name(GetDataName<Data>(data_message));
    int32_t data_size(static_cast<int32_t>(data_message.data().content.string().size()));
    auto put_op(std::make_shared<nfs::PutOrDeleteOp>(
        kPutSuccessCountMin_,
        [this, account_name, data_name, data_size, reply_functor](nfs::Reply overall_result) {
            HandlePutResult(overall_result, account_name, data_name, data_size, reply_functor,
                            is_unique_on_network<Data>());
        }));
    nfs_.Put(data,
             data_message.data_holder(),
             [put_op](std::string serialised_reply) {
                 nfs::HandlePutOrDeleteReply(put_op, serialised_reply);
             });
    return;
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    request_id = std::make_pair(data_message.message_id(), data_message.source().persona);
    reply = nfs::Reply(error, data_message.Serialise().data, reply_functor);
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    request_id = std::make_pair(data_message.message_id(), data_message.source().persona);
    reply = nfs::Reply(CommonErrors::unknown, data_message.Serialise().data, reply_functor);
  }
  try {
    SendReply(request_id, reply, reply_functor);
  }
  catch(...) {
    LOG(kWarning) << "Exception while forming reply.";
  }
}

template<typename Data>
void MaidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateDataMessage(data_message);
    auto data_name(GetDataName<Data>(data_message));
    DeleteFromAccount<Data>(detail::GetSourceMaidName(data_message), data_name, is_payable<Data>());
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
  SendReply(std::make_pair(data_message.message_id(), data_message.source().persona),
            nfs::Reply(CommonErrors::success, data_message.Serialise().data), reply_functor);
}

template<typename Data>
typename Data::name_type MaidAccountHolderService::GetDataName(
    const nfs::DataMessage& data_message) const {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return Data::name_type(crypto::Hash<crypto::SHA512>(data_message.data().name));
}

void MaidAccountHolderService::SendReply(
    const nfs::Accumulator<MaidName>::RequestIdentity& request_id,
    const nfs::Reply& reply,
    const routing::ReplyFunctor& reply_functor) {
  accumulator_.SetHandled(request_id, reply);
  reply_functor(reply.Serialise()->string());
}

template<typename Data>
void MaidAccountHolderService::PutToAccount(const MaidName& account_name,
                                            const typename Data::name_type& data_name,
                                            int32_t size,
                                            int32_t replication_count,
                                            std::true_type) {
  // TODO(Fraser#5#): 2013-02-08 - Consider having replication calculated by network.
  assert(data_message.data().action == nfs::DataMessage::Action::kPut);
  maid_account_handler_.PutData<Data>(
      detail::GetSourceMaidName(data_message),
      Data::name_type(data_message.data().name),
      static_cast<int32_t>(data_message.data().content.string().size()),
      replication_count);
}

template<typename Data>
void MaidAccountHolderService::DeleteFromAccount(const MaidName& account_name,
                                                 const typename Data::name_type& data_name,
                                                 std::true_type) {
  assert(data_message.data().action == nfs::DataMessage::Action::kDelete);
  maid_account_handler_.DeleteData<Data>(account_name, data_name);
}


template<typename Data>
on_scope_exit MaidAccountHolderService::GetScopeExitForPut(
    const MaidName& account_name,
    const typename Data::name_type& data_name) {
  on_scope_exit strong_guarantee([account_name, data_name]() {
    try {
      DeleteFromAccount(account_name, data_name);
    }
    catch(...) {}
  });
  return on_scope_exit;
}

template<typename Data>
MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                          const MaidName& account_name,
                                          const typename Data::name_type& data_name,
                                          int32_t data_size,
                                          routing::ReplyFunctor client_reply_functor,
                                          std::true_type) {
  try {
    if (overall_result.IsSuccess()) {
      PutToAccount<Data>(account_name, data_name, data_size, kDefaultPaymentFactor_,
                         is_payable<Data>());
      on_scope_exit strong_guarantee(GetScopeExitForPut(account_name, data_name));
      client_reply_functor(nfs::Reply(CommonErrors::success).Serialise()->string());
      strong_guarantee.Release();
    } else {
      client_reply_functor(overall_result.Serialise()->string());
    }
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
  }
}

template<typename Data>
MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                          const MaidName& account_name,
                                          const typename Data::name_type& data_name,
                                          int32_t data_size,
                                          routing::ReplyFunctor client_reply_functor,
                                          std::false_type) {
    // check with MM if data is unique.  If MM says already stored, check if we stored it.  If so success, else failure.

}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_HOLDER_INL_H_
