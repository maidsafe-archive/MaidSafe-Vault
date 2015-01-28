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
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/utils.h"

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
  MessageType::No_genereic_handler_is_available__Specialisation_is_required;
  return false;
}

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetResponseFromDataManagerToMpidNode& message,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Receiver& receiver);

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetCachedResponseFromCacheHandlerToMpidNode& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Receiver& receiver);

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetResponseFromDataManagerToDataGetter& message,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Receiver& receiver);

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetCachedResponseFromCacheHandlerToDataGetter& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Receiver& receiver);

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

template <>
bool DoCacheOperation(
    CacheHandlerService* service,
    const nfs::GetRequestFromMpidNodeToDataManager& message,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Receiver& receiver);

}  // namespace detail

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

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_HANDLERS_H_
