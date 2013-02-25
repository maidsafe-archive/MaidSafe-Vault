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
void DataHolderService::HandleDataMessage(const nfs::DataMessage& data_message,
                                   const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(data_message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  switch (data_message.data().action) {
    case nfs::DataMessage::Action::kPut:
      return HandlePutMessage<Data>(data_message, reply_functor);
    case nfs::DataMessage::Action::kGet:
      return HandleGetMessage<Data>(data_message, reply_functor);
    case nfs::DataMessage::Action::kDelete:
      return HandleDeleteMessage<Data>(data_message, reply_functor);
    default: {
      reply = nfs::Reply(VaultErrors::operation_not_supported, data_message.Serialise().data);
      std::lock_guard<std::mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(data_message, reply.error());
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<typename Data>
void DataHolderService::HandlePutMessage(const nfs::DataMessage& data_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    ValidatePutSender(data_message);
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    if (detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired_)) {
      permanent_data_store_.Put(data.name(), data_message.data().content);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(data_message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kPutRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kPutRequestsRequired_);
  }
}

template<typename Data>
void DataHolderService::HandleGetMessage(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateGetSender(data_message);
    typename Data::name_type data_name(data_message.data().name);
    nfs::Reply reply(CommonErrors::success, permanent_data_store_.Get(data_name));
    reply_functor(reply.Serialise()->string());
  } catch(const std::exception& /*ex*/) {
    reply_functor(nfs::Reply(CommonErrors::unknown,
                             data_message.Serialise().data).Serialise()->string());
  }
}

template<typename Data>
void DataHolderService::HandleDeleteMessage(const nfs::DataMessage& data_message,
                                     const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateDeleteSender(data_message);
    if (detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired_)) {
      permanent_data_store_.Delete(typename Data::name_type(data_message.data().name));
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(data_message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired_);
  }
  catch(...) {
    detail::AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired_);
  }
}

// Cache Handling
template<typename Data>
NonEmptyString DataHolderService::GetFromCache(const nfs::DataMessage& data_message) {
  return CacheGet<Data>(typename Data::name_type(data_message.data().name),
                        is_long_term_cacheable<Data>());
}

template<typename Data>
void DataHolderService::StoreInCache(const nfs::DataMessage& data_message) {
  CacheStore<Data>(typename Data::name_type(data_message.data().name), data_message.data().content,
                   is_long_term_cacheable<Data>());
}

template<typename Data>
NonEmptyString DataHolderService::CacheGet(const typename Data::name_type& name, std::false_type) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Get(name);
}

template<typename Data>
NonEmptyString DataHolderService::CacheGet(const typename Data::name_type& name, std::true_type) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Get(name);
}

template<typename Data>
void DataHolderService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            std::false_type) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(name, value);
}

template<typename Data>
void DataHolderService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            std::true_type) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(name, value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_INL_H_
