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

#include "maidsafe/vault/cache_handler/dispatcher.h"


namespace maidsafe {

namespace vault {

namespace detail {
  class PutToCacheVisitor;

  template<typename Sender, typename SourcePersonaType>
  class GetFromCacheVisitor;
}

class CacheHandlerService {
 public:
  typedef nfs::CacheableMessages PublicMessages;
  typedef CacheableMessages VaultMessages;
  typedef bool HandleMessageReturnType;

  CacheHandlerService(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);

  template <typename T>
  HandleMessageReturnType HandleMessage(const T& message, const typename T::Sender& sender,
                                        const typename T::Receiver& receiver);

  friend class detail::PutToCacheVisitor;
  template<typename Sender, typename SourcePersonaType> friend class detail::GetFromCacheVisitor;

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

  template <typename Data, typename SourcePersonaType>
  void SendGetResponse(const Data& data, const routing::SingleSource& sender);

  template <typename MessageType>
  bool ValidateSender(const MessageType& message, const typename MessageType::Sender& sender) const;

  routing::Routing& routing_;
  CacheHandlerDispatcher dispatcher_;
  DiskUsage cache_size_;
  data_store::DataStore<data_store::DataBuffer<DataNameVariant>> cache_data_store_;
  data_store::MemoryBuffer mem_only_cache_;
};

template <typename MessageType>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(const MessageType& /*message*/,
                                   const typename MessageType::Sender& /*sender*/,
                                   const typename MessageType::Receiver& /*receiver*/) {
//  MessageType::Specialisation_required;
  return false;
}

template <typename MessageType>
bool CacheHandlerService::ValidateSender(const MessageType& /*message*/,
                                         const typename MessageType::Sender& /*sender*/) const {
//  MessageType::Specialisation_required;
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


template <typename Data>
boost::optional<Data> CacheHandlerService::CacheGet(const typename Data::Name& data_name,
                                                    IsShortTermCacheable) {
  try {
    return boost::optional<Data>(
               Data(data_name,
                    typename Data::serialised_type(mem_only_cache_.Get(
                        GetDataNameVariant(Data::Tag::kValue, data_name.value)))));
  }
  catch (const std::exception&) {
    return boost::optional<Data>();
  }
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

template <typename Data, typename SourcePersonaType>
void CacheHandlerService::SendGetResponse(const Data& data, const routing::SingleSource& sender) {
  dispatcher_.SendGetResponse<Data, SourcePersonaType>(data, sender);
}

template <typename Data>
void CacheHandlerService::CacheStore(const Data& data, IsLongTermCacheable) {
  try {
    cache_data_store_.Store(GetDataNameVariant(Data::Tag::kValue, data.name().value),
                            data.Serialise().data);
  }
  catch (const std::exception&) {
    LOG(kError) << "Failed to store data in to the cache";
  }
}

template <typename Data>
void CacheHandlerService::CacheStore(const Data& data, IsShortTermCacheable) {
  try {
    mem_only_cache_.Store(GetDataNameVariant(Data::Tag::kValue, data.name().value),
                          data.Serialise().data);
  }
  catch (const std::exception&) {
    LOG(kError) << "Failed to store data in to the cache";
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_
