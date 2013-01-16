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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolder::HandleMessage(const nfs::Message& message,
                                      const routing::ReplyFunctor& reply_functor) {
  LOG(kInfo) << "received message at Data holder";
  // TODO(Team): Check the message content for validity with the MAID key we'll have eventually.
  switch (message.action_type()) {
    case nfs::ActionType::kGet:
      HandleGetMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kPut:
      HandlePutMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kDelete:
      HandleDeleteMessage<Data>(message, reply_functor);
      break;
    default: LOG(kError) << "Unhandled action type";
  }
}

template<typename Data>
void MaidAccountHolder::HandleGetMessage(nfs::Message /*message*/,
                                         const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void MaidAccountHolder::HandlePutMessage(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  if (!detail::NodeRangeCheck(routing_, message.source().node_id)) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  AdjustAccount<Data>(message, reply_functor, is_payable<Data>());
  nfs::OnError on_error_callback = [this](nfs::Message message) {
                                      this->OnPutErrorHandler<Data>(message);
                                   };
  nfs_.Put<Data>(message, on_error_callback);
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

template<typename Data>
void MaidAccountHolder::HandleDeleteMessage(const nfs::Message& message,
                                            const routing::ReplyFunctor& reply_functor) {
  if (!detail::NodeRangeCheck(routing_, message.source().node_id)) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

//  auto maid_account_it = std::find_if(maid_accounts_.begin(),
//                                      maid_accounts_.end(),
//                                      [&message] (const MaidAccount& maid_account) {
//                                        return maid_account.maid_name().data.string() ==
//                                               message.source().node_id.string();
//                                      });
//  if (maid_account_it == maid_accounts_.end()) {
//    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
//    return;
//  }

//  bool found_data_item(maid_account_it->Has(message.name()));
//  if (found_data_item) {
//    // Send message on to MetadataManager
//    nfs::OnError on_error_callback = [this] (nfs::Message message) {
//                                       this->OnDeleteErrorHandler<Data>(message);
//                                     };
//    nfs_.Delete<Data>(message, on_error_callback);
//    maid_account_it->Remove(message.name());
//  }

//  reply_functor(nfs::ReturnCode(found_data_item ? 0 : -1).Serialise()->string());
}

template<typename Data>
void MaidAccountHolder::AdjustAccount(const nfs::Message& /*message*/,
                                      const routing::ReplyFunctor& /*reply_functor*/,
                                      std::true_type) {
//  auto maid_account_it = std::find_if(maid_accounts_.begin(),
//                                      maid_accounts_.end(),
//                                      [&message] (const MaidAccount& maid_account) {
//                                        return maid_account.maid_name().data.string() ==
//                                            message.source().node_id.string();
//                                      });
//  if (maid_account_it == maid_accounts_.end()) {
//    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
//    return;
//  }

//  if (message.action_type() == nfs::ActionType::kPut) {
    // TODO(Team): BEFORE_RELEASE Check if we should allow the store based on PMID account
    // information
//    DataElements data_element(message.name(),
//                              static_cast<int32_t>(message.content().string().size()));
//    // TODO(Team): BEFORE_RELEASE Check if there will be case having a data_element bearing same
//    // data_id, and what shall be done in that case (i.e. reject PUT or update the data_element)
//    maid_account_it->PushDataElement(data_element);
//  } else {
//    assert(message.action_type() == nfs::ActionType::kDelete);
//    // TODO(Team): BEFORE_RELEASE Handle delete.
//  }
}

template<typename Data>
void MaidAccountHolder::OnPutErrorHandler(nfs::Message message) {
  if (detail::NodeRangeCheck(routing_, message.source().node_id))
    nfs_.Put<Data>(message,
                   [this] (nfs::Message message) {
                     this->OnPutErrorHandler<Data>(message);
                   });
}

template<typename Data>
void MaidAccountHolder::OnDeleteErrorHandler(nfs::Message message) {
  if (detail::NodeRangeCheck(routing_, message.source().node_id))
    nfs_.Delete<Data>(message,
                      [this] (nfs::Message message) {
                        this->OnDeleteErrorHandler<Data>(message);
                      });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_INL_H_
