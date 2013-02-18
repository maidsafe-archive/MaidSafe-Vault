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
#include "maidsafe/nfs/utils.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  auto request_id(std::make_pair(data_message.message_id(),
                                 MaidName(Identity(data_message.source().node_id.string()))));
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
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    MaidName account_name(detail::GetSourceMaidName(data_message));
    auto data_name(GetDataName<Data>(data_message));
    auto put_op(std::make_shared<nfs::PutOrDeleteOp>(
        kPutSuccessCountMin_,
        [this, account_name, data_name, reply_functor](nfs::Reply overall_result) {
            HandlePutResult<Data>(overall_result, account_name, data_name, reply_functor);
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
    request_id = std::make_pair(data_message.message_id(),
                                MaidName(Identity(data_message.source().node_id.string())));
    reply = nfs::Reply(error, data_message.Serialise().data);
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    request_id = std::make_pair(data_message.message_id(),
                                MaidName(Identity(data_message.source().node_id.string())));
    reply = nfs::Reply(CommonErrors::unknown, data_message.Serialise().data);
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
  SendReply(std::make_pair(data_message.message_id(),
                           MaidName(Identity(data_message.source().node_id.string()))),
            nfs::Reply(CommonErrors::success, data_message.Serialise().data), reply_functor);
}

template<typename Data>
typename Data::name_type MaidAccountHolderService::GetDataName(
    const nfs::DataMessage& data_message) const {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return typename Data::name_type(crypto::Hash<crypto::SHA512>(data_message.data().name));
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
                                            int32_t cost,
                                            std::true_type) {
  maid_account_handler_.PutData<Data>(account_name, data_name, cost);
}

template<typename Data>
void MaidAccountHolderService::DeleteFromAccount(const MaidName& account_name,
                                                 const typename Data::name_type& data_name,
                                                 std::true_type) {
  maid_account_handler_.DeleteData<Data>(account_name, data_name);
}


template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const MaidName& account_name,
                                               const typename Data::name_type& data_name,
                                               routing::ReplyFunctor client_reply_functor) {
  try {
    if (overall_result.IsSuccess()) {
      protobuf::Cost cost;
      cost.ParseFromString(overall_result.data().string());
      // TODO(Fraser#5#): 2013-02-13 - Have PutToAccount return percentage or amount remaining so
      // if it falls below a threshold, we can trigger getting updated account info from the PAHs
      // (not too frequently), & alert the client by returning an "error" via client_reply_functor.
      PutToAccount<Data>(account_name, data_name, cost.value(), is_payable<Data>());
      client_reply_functor(nfs::Reply(CommonErrors::success).Serialise()->string());
    } else {
      client_reply_functor(overall_result.Serialise()->string());
    }
  }
  catch(const maidsafe_error& me) {
    client_reply_functor(nfs::Reply(me).Serialise()->string());
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
    client_reply_functor(nfs::Reply(CommonErrors::unknown).Serialise()->string());
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_INL_H_
