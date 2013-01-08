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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"

#include "maidsafe/nfs/maid_account.h"

namespace maidsafe {

namespace vault {

template<typename Data>
void MaidAccountHolder::HandleMessage(const nfs::Message& message,
                                      const routing::ReplyFunctor& reply_functor) {
  LOG(kInfo) << "received message at Data holder";
  switch (message.action_type()) {
    case nfs::ActionType::kGet :
      HandleGetMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kPut :
      HandlePutMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kPost :
      HandlePostMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kDelete :
      HandleDeleteMessage<Data>(message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}

template<typename Data>
void MaidAccountHolder::HandleGetMessage(nfs::Message /*message*/,
                                         const routing::ReplyFunctor& /*reply_functor*/) {
  maidsafe::passport::PublicPmid p_pmid(pmid_);
  auto fetched_data = nfs_.Get<maidsafe::nfs::MaidAccount>(p_pmid.name());
  try {
    maidsafe::nfs::MaidAccount fetched_account = fetched_data.get();
  } catch (...) {
    LOG(kError) << "MaidAccountHolder - Failed to retrieve "
                << maidsafe::HexSubstr(p_pmid.name().data.string());
    return;
  }
}

template<typename Data>
void MaidAccountHolder::HandlePutMessage(const nfs::Message& /*message*/,
                                         const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void MaidAccountHolder::HandlePostMessage(const nfs::Message& /*message*/,
                                          const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void MaidAccountHolder::HandleDeleteMessage(const nfs::Message& /*message*/,
                                            const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_INL_H_
