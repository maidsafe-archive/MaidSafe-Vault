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

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/maid_account_holder/maid_account.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolder::HandleDataMessage(const nfs::DataMessage& data_message,
                                          const routing::ReplyFunctor& reply_functor) {
  // TODO(Team): Check the message content for validity with the MAID key we'll have eventually.
  switch (data_message.action_type()) {
    case nfs::DataMessage::ActionType::kGet:
      HandleGetMessage<Data>(data_message, reply_functor);
      break;
    case nfs::DataMessage::ActionType::kPut:
      HandlePutMessage<Data>(data_message, reply_functor);
      break;
    case nfs::DataMessage::ActionType::kDelete:
      HandleDeleteMessage<Data>(data_message, reply_functor);
      break;
    default: LOG(kError) << "Unhandled action type";
  }
}

template<typename Data>
void AccountHolder::HandleGetMessage(const nfs::DataMessage& /*data_message*/,
                                     const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void AccountHolder::HandlePutMessage(const nfs::DataMessage& data_message,
                                     const routing::ReplyFunctor& reply_functor) {
  if (!detail::NodeRangeCheck(routing_, data_message.source().node_id)) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  AdjustAccount<Data>(data_message, reply_functor, is_payable<Data>());
  nfs_.Put<Data>(data_message,
      [this](nfs::DataMessage data_msg) { this->OnPutErrorHandler<Data>(data_msg); });
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

template<typename Data>
void AccountHolder::HandleDeleteMessage(const nfs::DataMessage& data_message,
                                        const routing::ReplyFunctor& reply_functor) {
  if (!detail::NodeRangeCheck(routing_, data_message.source().node_id)) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  auto maid_account_it = std::find_if(maid_accounts_.begin(),
                                      maid_accounts_.end(),
                                      [&data_message] (const Account& account) {
                                        return maid_account.maid_name().data.string() ==
                                               data_message.source().node_id.string();
                                      });
  if (maid_account_it == maid_accounts_.end()) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  bool found_data_item(maid_account_it->Has(data_message.data().name));
  if (found_data_item) {
    // Send message on to MetadataManager
    nfs::DataMessage::OnError on_error_callback(
        [this] (nfs::DataMessage data_msg) { this->OnDeleteErrorHandler<Data>(data_msg); });
    nfs_.Delete<Data>(data_message, on_error_callback);
    maid_account_it->Remove(data_message.data().name);
  }

  reply_functor(nfs::ReturnCode(found_data_item ? 0 : -1).Serialise()->string());
}

template<typename Data>
void AccountHolder::AdjustAccount(const nfs::DataMessage& data_message,
                                  const routing::ReplyFunctor& reply_functor,
                                  std::true_type) {
  auto maid_account_it = std::find_if(maid_accounts_.begin(),
                                      maid_accounts_.end(),
                                      [&data_message] (const Account& account) {
                                        return maid_account.maid_name().data.string() ==
                                            data_message.source().node_id.string();
                                      });
  if (maid_account_it == maid_accounts_.end()) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  if (data_message.action_type() == nfs::DataMessage::ActionType::kPut) {
    // TODO(Team): BEFORE_RELEASE Check if we should allow the store based on PMID account
    // information
//    DataElements data_element(message.name(),
//                              static_cast<int32_t>(message.content().string().size()));
//    // TODO(Team): BEFORE_RELEASE Check if there will be case having a data_element bearing same
//    // data_id, and what shall be done in that case (i.e. reject PUT or update the data_element)
//    maid_account_it->PushDataElement(data_element);
  } else {
    assert(data_message.action_type() == nfs::DataMessage::ActionType::kDelete);
    // TODO(Team): BEFORE_RELEASE Handle delete.
  }
}

template<typename Data>
void AccountHolder::OnPutErrorHandler(nfs::DataMessage data_message) {
  nfs_.Put<Data>(data_message,
                 [this] (nfs::DataMessage data_msg) { this->OnPutErrorHandler<Data>(data_msg); });
}

template<typename Data>
void AccountHolder::OnDeleteErrorHandler(nfs::DataMessage data_message) {
  nfs_.Delete<Data>(data_message,
       [this] (nfs::DataMessage data_msg) { this->OnDeleteErrorHandler<Data>(data_msg); });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_HOLDER_INL_H_
