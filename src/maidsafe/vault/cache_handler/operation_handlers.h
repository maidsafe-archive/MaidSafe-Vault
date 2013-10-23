/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_HANDLERS_H_
#define MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_HANDLERS_H_

#include "maidsafe/common/types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/cache_handler/operation_visitors.h"

namespace maidsafe {

namespace vault {

class CacheHandlerService;

namespace detail {

template <typename MessageType>
bool DoCacheOperation(CacheHandlerService* service, const MessageType& message,
                      const typename MessageType::Sender& sender,
                      const typename MessageType::Receiver& receiver);

template <typename ValidateSender>
struct CacheOperationHandler {
  CacheOperationHandler(CacheHandlerService* service_in, ValidateSender validate_sender_in)
      : service(service_in),
        validate_sender(validate_sender_in) {}

  template <typename MessageType, typename Sender, typename Receiver>
  bool operator()(const MessageType& message, const Sender& sender, const Receiver& receiver);

 private:
  CacheHandlerService* service;
  ValidateSender validate_sender;
};

template <typename ValidateSender>
template <typename MessageType, typename Sender, typename Receiver>
bool CacheOperationHandler<ValidateSender>::operator()(
    const MessageType& message, const Sender& sender, const Receiver& receiver) {
  if (!validate_sender(message, sender))
    return false;
  return DoCacheOperation<MessageType>(service, message, sender, receiver);
}

template <typename MessageType>
bool DoCacheOperation(CacheHandlerService* /*service*/, const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/,
                      const typename MessageType::Receiver& /*receiver*/) {
  MessageType::Specialisation_Required;
  return false;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
  if (!message.contents->data)
    return false;
  auto data_name(detail::GetNameVariant(*message.contents));
  detail::PutToCacheVisitor put_to_cache(service, message.contents->data->content);
  boost::apply_visitor(put_to_cache, data_name);
  return true;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetCachedResponseFromCacheHandlerToMaidNode& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Receiver& /*receiver*/) {
  if (!message.contents->data)
    return false;
  auto data_name(detail::GetNameVariant(*message.contents->data));
  PutToCacheVisitor put_to_cache(service, message.contents->data->content);
  boost::apply_visitor(put_to_cache, data_name);
  return true;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetResponseFromDataManagerToDataGetter& message,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Receiver& /*receiver*/) {
  if (!message.contents->data)
    return false;
  auto data_name(detail::GetNameVariant(*message.contents->data));
  PutToCacheVisitor put_to_cache(service, message.contents->data->content);
  boost::apply_visitor(put_to_cache, data_name);
  return true;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetCachedResponseFromCacheHandlerToDataGetter& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Sender& /*sender*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Receiver& /*receiver*/) {
  if (!message.contents->data)
    return false;
  auto data_name(detail::GetNameVariant(*message.contents->data));
  PutToCacheVisitor put_to_cache(service, message.contents->data->content);
  boost::apply_visitor(put_to_cache, data_name);
  return true;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& /*receiver*/) {
  auto data_name(detail::GetNameVariant(*message.contents));
  GetFromCacheVisitor<typename nfs::GetRequestFromMaidNodeToDataManager::Sender,
                      typename nfs::GetRequestFromMaidNodeToDataManager::SourcePersona>
      get_from_cache(service, sender);
  return boost::apply_visitor(get_from_cache, data_name);
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& /*receiver*/) {
  auto data_name(detail::GetNameVariant(*message.contents));
  detail::GetFromCacheVisitor<typename nfs::GetRequestFromMaidNodeToDataManager::Sender,
                              typename nfs::GetRequestFromMaidNodeToDataManager::SourcePersona>
      get_from_cache(service, sender);
  return boost::apply_visitor(get_from_cache, data_name);
}

}  // detail

template <typename MessageType>
struct CacheOperationHandlerWrapper {
  typedef detail::CacheOperationHandler<typename detail::ValidateSenderType<MessageType>::type>
              TypedCacheOperationHandler;

  CacheOperationHandlerWrapper(
      CacheHandlerService* service,
      typename detail::ValidateSenderType<MessageType>::type validate_sender)
          : typed_cache_operation_handler(service, validate_sender) {}

  bool operator()(const MessageType& message, const typename MessageType::Sender& sender,
                  const typename MessageType::Receiver& receiver) {
    return typed_cache_operation_handler(message, sender, receiver);
  }

 private:
  TypedCacheOperationHandler typed_cache_operation_handler;
};

}  // namespace vault

}  // namespace maidsafe


#endif // MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_HANDLERS_H_

