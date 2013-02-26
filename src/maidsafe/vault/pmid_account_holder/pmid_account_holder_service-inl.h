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
  nfs::Reply reply(CommonErrors::success);
  if (accumulator_.CheckHandled(data_message, reply))
    return reply_functor(reply.Serialise()->string());

  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported);
    accumulator_.SetHandled(data_message, reply.error());
    reply_functor(reply.Serialise()->string());
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  try {
    ValidateSender(data_message);
    typename Data::name_type data_name(data_message.data().name);
    Data data(data_name, typename Data::serialised_type(data_message.data().content));
    int32_t size(static_cast<int32_t>(data_message.data().content.string().size()));
    PmidName account_name(data_message.data_holder());
    auto put_op(std::make_shared<nfs::OperationOp>(
        1,
        [this, account_name, data_name, size, reply_functor](nfs::Reply overall_result) {
            HandlePutResult<Data>(overall_result, account_name, data_name, size,
                                  reply_functor);
        }));
    nfs_.Put(data_message.data_holder(),
             data,
             [put_op](std::string serialised_reply) {
                 nfs::HandleOperationReply(put_op, serialised_reply);
             });
    return;
  }
  catch(const maidsafe_error& error) {
    LOG(kError) << "Failure putting data to account: " << error.what();
    reply = nfs::Reply(error);
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failure putting data to account: " << e.what();
    reply = nfs::Reply(CommonErrors::unknown);
  }
  accumulator_.SetHandled(data_message, reply.error());
  reply_functor(reply.Serialise()->string());
}

template<typename Data>
void PmidAccountHolderService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    typename Data::name_type data_name(data_message.data().name);
    pmid_account_handler_.DeleteData<Data>(data_message.data_holder(), data_name);
    SendDataMessage<Data>(data_message);
  }
  catch(const std::system_error& error) {
    LOG(kError) << "Failure deleting data from account: " << error.what();
    // Always return succeess for Deletes
  }
  nfs::Reply reply(CommonErrors::success);
  accumulator_.SetHandled(data_message, reply.error());
  reply_functor(reply.Serialise()->string());
}

template<typename Data>
void PmidAccountHolderService::SendDataMessage(const nfs::DataMessage& data_message) {
  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
//    typename Data::name_type data_name(data_message.data().name);
//    Data data(data_name, typename Data::serialised_type(data_message.data().content));
//    int32_t size(static_cast<int32_t>(data_message.data().content.string().size()));
//    nfs_.Put<Data>(data_message.data_holder(),
//                   data,
//                   [this, data_name, size](nfs::DataMessage data_msg) {
//                     pmid_account_handler_.PutData<Data>(data_msg.data_holder(), data_name, size);
//                   });
  } else {
//    assert(data_message.data().action == nfs::DataMessage::Action::kDelete);
//    nfs_.Delete<Data>(data_message,
//                      [this](nfs::DataMessage data_msg) {
//                        detail::RetryOnPutOrDeleteError<PmidAccountHolderNfs, Data>(routing_,
//                                                                                    nfs_,
//                                                                                    data_msg);
//                      });
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePutResult(const nfs::Reply& data_holder_result,
                                               const PmidName& account_name,
                                               const typename Data::name_type& data_name,
                                               int32_t size,
                                               routing::ReplyFunctor mm_reply_functor) {
  try {
    if (data_holder_result.IsSuccess()) {
      pmid_account_handler_.PutData<Data>(account_name, data_name, size);
      mm_reply_functor(nfs::Reply(CommonErrors::success).Serialise()->string());
    } else {
      mm_reply_functor(data_holder_result.Serialise()->string());
    }
  }
  catch(const maidsafe_error& me) {
    mm_reply_functor(nfs::Reply(me).Serialise()->string());
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
    mm_reply_functor(nfs::Reply(CommonErrors::unknown).Serialise()->string());
  }
}



}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
