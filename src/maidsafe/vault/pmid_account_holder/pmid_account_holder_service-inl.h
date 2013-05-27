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
#include "maidsafe/vault/unresolved_element.h"

namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data, nfs::MessageAction action>
PmidAccountUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                                 const NodeId& this_id) {
  static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return MaidAccountUnresolvedEntry(
      std::make_pair(DataNameVariant(Data::name_type, message.data().name), action),
      message.data().content.string().size(),
      this_id);
}

PmidName GetPmidName(const nfs::Message& message);

}  // namespace detail

template<typename Data>
void PmidAccountHolderService::HandleMessage(const nfs::Message& message,
                                             const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  if (message.data().action == nfs::MessageAction::kPut) {
    HandlePut<Data>(message, reply_functor);
  } else if (message.data().action == nfs::MessageAction::kDelete) {
    HandleDelete<Data>(message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, reply.error());
    reply_functor(reply.Serialise()->string());
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePut(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateSender(message);
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired_)) {
      pmid_account_handler_.Put<Data>(message.data_holder(), data.name(),
          static_cast<int32_t>(message.data().content.string().size()));
      auto put_op(std::make_shared<nfs::OperationOp>(
           kPutRepliesSuccessesRequired_,
           [this, message, reply_functor](nfs::Reply overall_result) {
               this->template HandlePutResult<Data>(overall_result, message, reply_functor);
           }));
      nfs_.Put(message.data_holder(),
               data,
               [put_op](std::string serialised_reply) {
                   nfs::HandleOperationReply(put_op, serialised_reply);
               });
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kPutRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kPutRequestsRequired_);
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::Message& message,
                                               routing::ReplyFunctor /*reply_functor*/) {
  if (overall_result.IsSuccess()) {
    nfs::Reply reply(CommonErrors::success);
    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message);
  }
}

template<typename Data>
void PmidAccountHolderService::HandleDelete(const nfs::Message& message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateSender(message);
    typename Data::name_type data_name(message.data().name);
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired_)) {
      pmid_account_handler_.Delete<Data>(message.data_holder(), data_name);
      nfs_.Delete<Data>(message.data_holder(), data_name, nullptr);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired_);
  }
}

template<typename Data, nfs::MessageAction action>
void PmidAccountHolderService::AddLocalUnresolvedEntryThenSync(const nfs::Message& message) {
  auto account_name(detail::GetPmidName(message));
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(
      message, static_cast<int32_t>(message.data().content.string().size()), routing_.kNodeId()));
//  pmid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
//  Sync(account_name);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
