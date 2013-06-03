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

#ifndef MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void DataHolderService::HandleMessage(const nfs::Message& message,
                                      const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  switch (message.data().action) {
    case nfs::MessageAction::kPut:
      return HandlePutMessage<Data>(message, reply_functor);
    case nfs::MessageAction::kGet:
      return HandleGetMessage<Data>(message, reply_functor);
    case nfs::MessageAction::kDelete:
      return HandleDeleteMessage<Data>(message, reply_functor);
    default: {
      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
      std::lock_guard<std::mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, reply);
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<typename Data>
void DataHolderService::HandlePutMessage(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidatePutSender(message);
#endif
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired)) {
      permanent_data_store_.Put(data.name(), message.data().content);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kPutRequestsRequired);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kPutRequestsRequired);
  }
}

template<typename Data>
void DataHolderService::HandleGetMessage(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidateGetSender(message);
#endif
    typename Data::name_type data_name(message.data().name);
    nfs::Reply reply(CommonErrors::success, permanent_data_store_.Get(data_name));
    reply_functor(reply.Serialise()->string());
  } catch(const std::exception& /*ex*/) {
    reply_functor(nfs::Reply(CommonErrors::unknown,
                             message.Serialise().data).Serialise()->string());
  }
}

template<typename Data>
void DataHolderService::HandleDeleteMessage(const nfs::Message& message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidateDeleteSender(message);
#endif
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired)) {
      permanent_data_store_.Delete(typename Data::name_type(message.data().name));
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired);
  }
}

template<typename Data>
NonEmptyString DataHolderService::GetFromCache(const nfs::Message& message) {
  return GetFromCache<Data>(message, is_cacheable<Data>());
}

template<typename Data>
NonEmptyString DataHolderService::GetFromCache(const nfs::Message& message, IsCacheable) {
  return CacheGet<Data>(typename Data::name_type(message.data().name),
                        is_long_term_cacheable<Data>());
}

template<typename Data>
NonEmptyString DataHolderService::GetFromCache(const nfs::Message& /*message*/,
                                               IsNotCacheable) {
  return NonEmptyString();
}

template<typename Data>
NonEmptyString DataHolderService::CacheGet(const typename Data::name_type& name,
                                           IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Get(name);
}

template<typename Data>
NonEmptyString DataHolderService::CacheGet(const typename Data::name_type& name,
                                           IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Get(name);
}

template<typename Data>
void DataHolderService::StoreInCache(const nfs::Message& message) {
  StoreInCache<Data>(message, is_cacheable<Data>());
}

template<typename Data>
void DataHolderService::StoreInCache(const nfs::Message& message, IsCacheable) {
  CacheStore<Data>(typename Data::name_type(message.data().name), message.data().content,
                   is_long_term_cacheable<Data>());
}

template<typename Data>
void DataHolderService::StoreInCache(const nfs::Message& /*message*/, IsNotCacheable) {}

template<typename Data>
void DataHolderService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(name, value);
}

template<typename Data>
void DataHolderService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(name, value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_INL_H_
