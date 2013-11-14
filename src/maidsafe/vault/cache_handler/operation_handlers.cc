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


#include "maidsafe/vault/cache_handler/operation_handlers.h"

#include "maidsafe/vault/cache_handler/operation_visitors.h"

namespace maidsafe {

namespace vault {

class CacheHandlerService;

namespace detail {


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
  typedef nfs::GetRequestFromMaidNodeToDataManager::SourcePersona SourcePersonaType;
  auto data_name(detail::GetNameVariant(*message.contents));
  detail::Requestor<SourcePersonaType> requestor(sender.data);
  GetFromCacheVisitor<detail::Requestor<SourcePersonaType>> get_from_cache(service, requestor);
  return boost::apply_visitor(get_from_cache, data_name);
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& /*receiver*/) {
  typedef nfs::GetRequestFromMaidNodeToDataManager::SourcePersona SourcePersonaType;
  auto data_name(detail::GetNameVariant(*message.contents));
  detail::Requestor<SourcePersonaType> requestor(sender.data);
  detail::GetFromCacheVisitor<detail::Requestor<SourcePersonaType>> get_from_cache(service,
                                                                                   requestor);
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
