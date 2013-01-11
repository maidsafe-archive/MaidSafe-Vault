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

#ifndef MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_INL_H_
#define MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void DataHolder::HandleMessage(const nfs::Message& message,
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
void DataHolder::HandleGetMessage(const nfs::Message& message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::Message response(message.action_type(),
                          message.destination_persona_type(),
                          message.source(),
                          message.data_type(),
                          message.name(),
                          permanent_data_store_.Get(typename Data::name_type(message.name())),
                          asymm::Signature());
    reply_functor(response.Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandlePutMessage(const nfs::Message& message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    permanent_data_store_.Store(typename Data::name_type(message.name()), message.content());
    reply_functor(nfs::ReturnCode(0).Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandlePostMessage(const nfs::Message& /*message*/,
                                   const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void DataHolder::HandleDeleteMessage(const nfs::Message& message,
                                     const routing::ReplyFunctor& reply_functor) {
  permanent_data_store_.Delete(typename Data::name_type(message.name()));
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

// Cache Handling
template<typename Data>
NonEmptyString DataHolder::GetFromCache(const nfs::Message& message) {
  return CacheGet<Data>(typename Data::name_type(message.name()), is_long_term_cacheable<Data>());
}

template<typename Data>
void DataHolder::StoreInCache(const nfs::Message& message) {
  CacheStore<Data>(typename Data::name_type(message.name()), message.content(),
                   is_long_term_cacheable<Data>());
}

template<typename Data>
NonEmptyString DataHolder::CacheGet(const typename Data::name_type& name, std::false_type) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Get(name);
}

template<typename Data>
NonEmptyString DataHolder::CacheGet(const typename Data::name_type& name, std::true_type) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Get(name);
}

template<typename Data>
void DataHolder::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            std::false_type) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(name, value);
}

template<typename Data>
void DataHolder::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            std::true_type) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(name, value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_INL_H_
