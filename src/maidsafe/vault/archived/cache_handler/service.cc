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

#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/cache_handler/operation_visitors.h"
#include "maidsafe/vault/cache_handler/operation_handlers.h"

namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(100);  // size in elements
DiskUsage cache_size = DiskUsage(200);

}  // unnamed namespace

CacheHandlerService::CacheHandlerService(routing::Routing& routing,
                                         const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      memory_cache_mutex_(),
      dispatcher_(routing),
      cache_size_(cache_size),
      cache_data_store_(vault_root_dir / "cache" / "cache", DiskUsage(cache_usage)),
      mem_only_cache_(detail::Parameters::temporary_store_size,
                      detail::Parameters::temporary_store_time_to_live) {
  routing_.kNodeId();
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToMpidNode& message,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Receiver& receiver) {
  typedef nfs::GetResponseFromDataManagerToMpidNode MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToMpidNode& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Receiver& receiver) {
  typedef nfs::GetCachedResponseFromCacheHandlerToMpidNode MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToDataGetter& message,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Receiver& receiver) {
  typedef nfs::GetResponseFromDataManagerToDataGetter MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToDataGetter& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Receiver& receiver) {
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromMpidNodeToDataManager& message,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromMpidNodeToDataManager MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver) {
  typedef nfs::GetRequestFromDataGetterToDataManager MessageType;
  return CacheOperationHandlerWrapper<MessageType>(
             this, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                   })(message, sender, receiver);
}

}  // namespace vault

}  // namespace maidsafe
