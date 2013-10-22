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
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/cache_handler/operation_handlers.h"



namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(100);  // size in elements
DiskUsage cache_size = DiskUsage(200);

}

CacheHandlerService::CacheHandlerService(routing::Routing& routing,
                                         const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      dispatcher_(routing),
      cache_size_(cache_size),
      cache_data_store_(cache_usage, DiskUsage(cache_usage), nullptr,
                        vault_root_dir / "cache" / "cache"),
      mem_only_cache_(mem_only_cache_usage) {
  routing_.kNodeId();
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
  if (!message.contents->data)
    return false;
  auto data_name(detail::GetNameVariant(*message.contents));
  PutToCacheVisitor put_to_cache(this, message.contents->data->content);
  boost::apply_visitor(put_to_cache, data_name);
  return true;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToMaidNode& /*message*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToMaidNode::Receiver& /*receiver*/) {
  return true;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetResponseFromDataManagerToDataGetter& /*message*/,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToDataGetter::Receiver& /*receiver*/) {
  return true;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetCachedResponseFromCacheHandlerToDataGetter& /*message*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Sender& /*sender*/,
    const typename nfs::GetCachedResponseFromCacheHandlerToDataGetter::Receiver& /*receiver*/) {
  return true;
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& /*receiver*/) {
  auto data_name(detail::GetNameVariant(*message.contents));
  GetFromCacheVisitor<typename nfs::GetRequestFromMaidNodeToDataManager::Sender>
      get_from_cache(this, sender);
  return boost::apply_visitor(get_from_cache, data_name);
}

template <>
CacheHandlerService::HandleMessageReturnType
CacheHandlerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& /*receiver*/) {
  auto data_name(detail::GetNameVariant(*message.contents));
  GetFromCacheVisitor<typename nfs::GetRequestFromMaidNodeToDataManager::Sender>
      get_from_cache(this, sender);
  return boost::apply_visitor(get_from_cache, data_name);
}

}  // namespace vault

}  // namespace maidsafe
