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
  }
  catch(...) {
    LOG(kError) << "MaidAccountHolder - Failed to retrieve "
                << maidsafe::HexSubstr(p_pmid.name().data.string());
    return;
  }
}

template<typename Data>
void MaidAccountHolder::HandlePutMessage(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  if (!routing_.IsNodeIdInGroupRange(message.source().data.node_id)) {  // provisional call to Is..
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }
  bool is_registered(false);
  for (auto& maid_account : maid_accounts_) {
    for (auto& pmid_total : maid_account.pmid_totals) {
      if (NodeId(pmid_total.registration.pmid_id) == message.source()->node_id &&
            pmid_total.registration.register_) {
        auto get_key_future([&message](std::future<passport::PublicMaid> future) {
            try {
              passport::PublicMaid public_maid(future.get());
              asymm::PublicKey public_key(public_maid.public_key());
              if (!asymm::CheckSignature(message.content(), message.signature(), public_key))
                ThrowError(NfsErrors::invalid_signature);
            }
            catch(...) {
              LOG(kError) << "Exception thrown getting public key.";
              ThrowError(NfsErrors::invalid_parameter);
            }
        });
        public_key_getter_.HandleGetKey<passport::PublicMaid>(
            passport::PublicMaid::name_type(maid_account.maid_id), get_key_future);
        is_registered = true;
        break;
      }
    }
    if (is_registered)
      break;
  }
  if (!is_registered) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }
  if (is_payable<Data>::value) {
  } else {
    try {
      // nfs_.Put(message, reply_functor);
    }
    catch(const std::exception&) {
      reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    }
  }
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

template<typename Data>
void MaidAccountHolder::HandlePostMessage(const nfs::Message& /*message*/,
                                          const routing::ReplyFunctor& /*reply_functor*/) {
// HandleNewComer(p_maid);
}

template<typename Data>
void MaidAccountHolder::HandleDeleteMessage(const nfs::Message& /*message*/,
                                            const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_INL_H_
