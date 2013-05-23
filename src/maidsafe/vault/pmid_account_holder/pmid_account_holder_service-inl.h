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

namespace {

template<typename Data, nfs::MessageAction action>
PmidAccountUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                                 int32_t size,
                                                 const NodeId& this_id) {
  static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return PmidAccountUnresolvedEntry(
      std::make_pair(DataNameVariant(GetDataName<Data>(message)), action), size, this_id);
}

}

template<typename Data>
void PmidAccountHolderService::HandleMessage(const Message& message,
                                             const ReplyFunctor& reply_functor) {
  ValidateDataSender(message);
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
    SendReplyAndAddToAccumulator(message, reply_functor, reply);
  }
}

template<typename Data>
void PmidAccountHolderService::HandlePut(const Message& message,
                                         const ReplyFunctor& reply_functor) {
  try {
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    auto account_name(detail::GetPmidAccountName(message));
    auto size(message.data().content.string().size());
    pmid_account_handler_.CreateAccount<Data>(account_name, detail::can_create_account<Data>());

    auto put_op(std::make_shared<nfs::OperationOp>(
        kPutRepliesSuccessesRequired_,
        [this, message, reply_functor](nfs::Reply overall_result) {
            this->HandlePutResult<Data>(overall_result, message, reply_functor,
                                        is_unique_on_network<Data>());
        }));

    nfs_.Put(data,
             message.data_holder(),
             [put_op](std::string serialised_reply) {
                 nfs::HandleOperationReply(put_op, serialised_reply);
             });
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
  nfs::Reply reply(return_code, message.Serialise().data);
  SendReplyAndAddToAccumulator(message, reply_functor, reply);
}

template<typename Data>
void PmidAccountHolderService::HandleDelete(const Message& message,
                                            const ReplyFunctor& reply_functor) {
  try {
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

template<typename Data>
void PmidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::Message& message,
                                               routing::ReplyFunctor reply_functor) {
  if (overall_result.IsSuccess()) {
    nfs::Reply reply(CommonErrors::success);
    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message);
    SendReplyAndAddToAccumulator(message, reply_functor, reply);
  } else {
    SendReplyAndAddToAccumulator(message, reply_functor, overall_result);
  }
}

template<typename Data, nfs::MessageAction action>
void PmidAccountHolderService::AddLocalUnresolvedEntryThenSync(const nfs::Message& message) {
  auto account_name(detail::GetPmidAccountName(message));
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(
          message, message.data().content.size(), routing_.kNodeId()));
  pmid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
  Sync(account_name);
}

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_SERVICE_INL_H_
