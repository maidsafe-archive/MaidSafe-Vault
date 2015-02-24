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

#include "maidsafe/common/containers/lru_cache.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/cache_handler/dispatcher.h"
#include "maidsafe/vault/chunk_store.h"
#include "maidsafe/vault/message_types.h"


namespace maidsafe {

namespace vault {

namespace detail {

class PutToCacheVisitor;

template<typename RequestorType>
class GetFromCacheVisitor;

}

namespace test {

  class CacheHandlerServiceTest;
}

class CacheHandlerService {
 public:
  typedef nfs::CacheableMessages PublicMessages;
  typedef void VaultMessages;
  typedef bool HandleMessageReturnType;

  CacheHandlerService(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);

  template <typename MessageType>
  HandleMessageReturnType HandleMessage(const MessageType& message,
                                        const typename MessageType::Sender& sender,
                                        const typename MessageType::Receiver& receiver);

  friend class detail::PutToCacheVisitor;
  template<typename RequestorType> friend class detail::GetFromCacheVisitor;
  friend class test::CacheHandlerServiceTest;

 private:
  typedef std::true_type IsLongTermCacheable;
  typedef std::false_type IsShortTermCacheable;

  template <typename Data>
  boost::optional<Data> CacheGet(const typename Data::Name& data_name, IsShortTermCacheable);

  template <typename Data>
  boost::optional<Data> CacheGet(const typename Data::Name& data_name, IsLongTermCacheable);

  template <typename Data>
  void CacheStore(const Data& data, IsLongTermCacheable);

  template <typename Data>
  void CacheStore(const Data& data, IsShortTermCacheable);

  template <typename Data, typename RequestorType>
  void SendGetResponse(const Data& data,  const nfs::MessageId message_id,
                       const RequestorType& requestor);

  template <typename MessageType>
  bool ValidateSender(const MessageType& message, const typename MessageType::Sender& sender) const;

  routing::Routing& routing_;
  std::mutex memory_cache_mutex_;
  CacheHandlerDispatcher dispatcher_;
  DiskUsage cache_size_;
  ChunkStore cache_data_store_;
  LruCache<vault::Key, NonEmptyString> mem_only_cache_;
};

template <typename MessageType>
typename CacheHandlerService::HandleMessageReturnType CacheHandlerService::HandleMessage(
    const MessageType& /*message*/,
    const typename MessageType::Sender& /*sender*/,
    const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_genereic_handler_is_available__Specialisation_is_required;
  return false;
}

template <typename MessageType>
bool CacheHandlerService::ValidateSender(const MessageType& /*message*/,
                                         const typename MessageType::Sender& /*sender*/) const {
//  MessageType::No_genereic_handler_is_available__Specialisation_is_required;
// BEFORE_RELEASE
  return true;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToMpidNode& message,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToMpidNode::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToMpidNode& message,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Sender& sender,
    const typename nfs::GetCachedResponseFromCacheHandlerToMpidNode::Receiver& receiver);

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
    const nfs::GetRequestFromMpidNodeToDataManager& message,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMpidNodeToDataManager::Receiver& receiver);

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

template <typename Data>
boost::optional<Data> CacheHandlerService::CacheGet(const typename Data::Name& data_name,
                                                    IsShortTermCacheable) {
  using LruCacheGetResult = boost::expected<NonEmptyString, maidsafe_error>;
  LruCacheGetResult get_result;
  {
    std::lock_guard<decltype(memory_cache_mutex_)> lock(memory_cache_mutex_);
    get_result = mem_only_cache_.Get(Key(data_name.value, Data::Tag::kValue));
  }
  if (get_result)
    return boost::optional<Data>(Data(data_name,
                                 typename Data::serialised_type(get_result.value())));
  return boost::optional<Data>();
}

template <typename Data>
boost::optional<Data> CacheHandlerService::CacheGet(const typename Data::Name& data_name,
                                                    IsLongTermCacheable) {
  try {
    return boost::optional<Data>(
               Data(data_name,
                    typename Data::serialised_type(cache_data_store_.Get(
                        GetDataNameVariant(Data::Tag::kValue, data_name.value)))));
  }
  catch (const std::exception&) {
    return boost::optional<Data>();
  }
}

template <typename Data, typename RequestorType>
void CacheHandlerService::SendGetResponse(const Data& data, const nfs::MessageId message_id,
                                          const RequestorType& requestor) {
  LOG(kVerbose) << "CacheHandlerService::SendGetResponse";
  dispatcher_.SendGetResponse(data, message_id, requestor);
}

template <typename Data>
void CacheHandlerService::CacheStore(const Data& data, IsLongTermCacheable) {
  try {
    LOG(kVerbose) << "CacheHandlerService::CacheStore: cache_data_store: "
                  << HexSubstr(data.name().value) << " on " << DebugId(routing_.kNodeId());
    cache_data_store_.Put(GetDataNameVariant(Data::Tag::kValue, data.name().value),
                          data.Serialise().data);
  }
  catch (const std::exception&) {
    LOG(kError) << "Failed to store data in to the cache";
  }
}

template <typename Data>
void CacheHandlerService::CacheStore(const Data& data, IsShortTermCacheable) {
  LOG(kVerbose) << "CacheHandlerService::CacheStore: mem_only_cache";
  {
    std::lock_guard<decltype(memory_cache_mutex_)> lock(memory_cache_mutex_);
    mem_only_cache_.Add(Key(data.name().value, Data::Tag::kValue), data.Serialise().data);
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_
