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

template<typename Data, typename MessageType>
void HandleMessage(const MessageType& message,
                   const routing::ReplyFunctor& reply_functor) {
//  if (nfs::DataMessage::message_type_identifier == message.type()) {  // FIXME Prakash
    HandleDataMessage(message, reply_functor);
//  }
}

template<typename Data>
void DataHolder::HandleDataMessage(const nfs::DataMessage& data_message,
                                   const routing::ReplyFunctor& reply_functor) {
  LOG(kInfo) << "received message at Data holder";
  switch (data_message.data().action) {
    case nfs::DataMessage::Action::kGet :
      HandleGetMessage<Data>(data_message, reply_functor);
      break;
    case nfs::DataMessage::Action::kPut :
      HandlePutMessage<Data>(data_message, reply_functor);
      break;
    case nfs::DataMessage::Action::kDelete :
      HandleDeleteMessage<Data>(data_message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}

template<typename Data>
void DataHolder::HandleGetMessage(const nfs::DataMessage& data_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
  // TODO(Fraser#5#): 2013-01-18 - Take version into account properly here
    nfs::DataMessage response(
        data_message.next_persona(),
        data_message.source(),
        nfs::DataMessage::Data(
            data_message.data().type,
            data_message.data().name,
            permanent_data_store_.Get(typename Data::name_type(data_message.data().name)),
            data_message.data().action));
    reply_functor(response.Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandlePutMessage(const nfs::DataMessage& data_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    permanent_data_store_.Store(typename Data::name_type(data_message.data().name),
                                data_message.data().content);
    reply_functor(nfs::ReturnCode(0).Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandleDeleteMessage(const nfs::DataMessage& data_message,
                                     const routing::ReplyFunctor& reply_functor) {
  permanent_data_store_.Delete(typename Data::name_type(data_message.data().name));
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

//  template<typename Data>
//  void DataHolder::HandlePostMessage(const nfs::Message& /*message*/,
//                                     const routing::ReplyFunctor& /*reply_functor*/) {
//  // no op
//  }

// Cache Handling
template<typename Data>
NonEmptyString DataHolder::GetFromCache(const nfs::DataMessage& data_message) {
  return CacheGet<Data>(typename Data::name_type(data_message.data().name),
                        is_long_term_cacheable<Data>());
}

template<typename Data>
void DataHolder::StoreInCache(const nfs::DataMessage& data_message) {
  CacheStore<Data>(typename Data::name_type(data_message.data().name), data_message.data().content,
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
