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

#ifndef MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_
#define MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_

#include <type_traits>

#include "maidsafe/data_store/data_store.h"
#include "maidsafe/data_store/memory_buffer.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/nfs/message_types.h"


namespace maidsafe {

namespace vault {

namespace {

class LongTermCacheableVisitor : public boost::static_visitor<bool> {
 public:
  template <typename Data>
  void operator()() {
    return is_long_term_cacheable<Data>::value;
  }
};

class CacheableVisitor : public boost::static_visitor<bool> {
 public:
  template <typename Data>
  void operator()() {
    return is_cacheable<Data>::value;
  }
};

}  // noname namespace


class CacheHandlerService {
 public:
  typedef nfs::CacheableMessages PublicMessages;
  typedef CacheableMessages VaultMessages;
  typedef bool HandleMessageReturnType;
  CacheHandlerService(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);

  template <typename T>
  HandleMessageReturnType HandleMessage(const T& message, const typename T::Sender& sender,
                                        const typename T::Receiver& receiver);

 private:
  typedef std::true_type IsCacheable, IsLongTermCacheable;
  typedef std::false_type IsNotCacheable, IsShortTermCacheable;

  template <typename Data>
  void HandleStore(const Data& data);

  // NB - for GetFromCacheFromDataManagerToDataManager messages, validate that the sender is "close"
  // to the data.name() being requested.
  template <typename T>
  bool CacheGet(const T& message, const typename T::Sender& sender,
                const typename T::Receiver& receiver, IsShortTermCacheable);

  // NB - for GetFromCacheFromDataManagerToDataManager messages, validate that the sender is "close"
  // to the data.name() being requested.
  template <typename T>
  bool CacheGet(const T& message, const typename T::Sender& sender,
                const typename T::Receiver& receiver, IsLongTermCacheable);

  template <typename T>
  void CacheStore(const T& message, const DataNameVariant& data_name, IsShortTermCacheable);

  template <typename T>
  void CacheStore(const T& message, const DataNameVariant& data_name, IsLongTermCacheable);

  routing::Routing& routing_;
  DiskUsage cache_size_;
  data_store::DataStore<data_store::DataBuffer<DataNameVariant>> cache_data_store_;
  data_store::MemoryBuffer mem_only_cache_;
};

template <typename T>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(const T& /*message*/, const typename T::Sender& /*sender*/,
                                   const typename T::Receiver& /*receiver*/) {
//  T::No_General_Implementation_Available;
  return false;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToMaidNode& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToDataGetter& message,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToDataGetter& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_
