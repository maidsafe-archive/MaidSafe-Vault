/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

template<>
void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
    const nfs::GetRequestFromDataManagerToPmidNode& message,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
  typedef nfs::GetResponseFromPmidNodeToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  {
    std::lock_guard<mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message))
      return;
  }
  try {
#ifndef TESTING
    ValidateGetSender(sender);
#endif
    auto data_name(nfs_vault::DataName(message.contents));
    auto content(permanent_data_store_.Get(data_name));
    NfsMessage nfs_message(nfs::DataNameAndContent(data_name.type, data_name.raw_name, content));
    RoutingMessage message(nfs_message.Serialise(),
                           NfsMessage::Sender(routing::SingleId(routing_.KnodeId())),
                           sender);
    routing_.Send(message);
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, sender);
    }
  } catch (const maidsafe_error& error) {
    NfsMessage nfs_message(nfs::DataNameAndReturnCode(data_name.type, error.code));
    RoutingMessage message(nfs_message.Serialise(),
                           NfsMessage::Sender(routing::SingleId(routing_.KnodeId())),
                           sender);
    routing_.Send(message);
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, sender);
    }
  } catch(const std::exception& /*ex*/) {
  }
}

template<>
void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  {
    std::lock_guard<mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, sender))
      return;
  }
  try {
#ifndef TESTNG
  ValidateDeleteSender(sender);
#endif
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      if (!accumulator_.AddPendingRequest(message, sender, kDeleteRequestsRequired))
        return;
      permanent_data_store_.Delete(nfs::DataName(message.Contents));
      accumulator_.SetHandled(message, sender);
    }
  } catch(const std::exception& /*ex*/) {
    std::lock_guard<mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, sender);
  }
}

template<>
void PmidNodeService::HandleMessage<nfs::PutRequestFromPmidManagerToPmidNode>(
    const nfs::PutRequestFromPmidManagerToPmidNode& message,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  {
    std::lock_guard<mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, sender))
      return;
  }
  try {
#ifndef TESTNG
  ValidatePutSender(sender);
#endif
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      if (!accumulator_.AddPendingRequest(message, sender, kPutRequestsRequired))
        return;
    }
    permanent_data_store_.Put(nfs::DataName(message.Contents.name), message.Contents.content);
    typedef Nfs::PutResponseFromPmidNodeToPmidManager NfsMessage;
    typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
    NfsMessage nfs_message(nfs::DataName(message.Contents.name));
    RoutingMessage routing_message(nfs_message.Serialise(),
                                   NfsMessage::Sender(routing::SingleId(routing_.KnodeId())),
                                   NfsMessage::Receiver(routing::GroupId(routing_.KnodeId())));
    routing_.Send(routing_message);
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, sender);
    }
  } catch(const maidsafe_error& error) {
    typedef Nfs::PutResponseFromPmidNodeToDataManager NfsMessage;
    typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
    NfsMessage nfs_message(nfs::DataNameAndContentAndReturnCode(message.Contents.name,
                                                                message.Contents.content,
                                                                error.code));
    RoutingMessage routing_message(nfs_message.Serialise(),
                                   NfsMessage::Sender(routing::SingleId(routing_.KnodeId())),
                                   NfsMessage::Receiver(routing::GroupId(message.Contents.name)));
    routing_.Send(routing_message);
    std::lock_guard<mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, sender);
  }
}

template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
    return GetFromCache(message, sender, receiver,
                        is_cacheable<nfs::DataName(message.Contents>()));
}

template<>
bool GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
    const nfs::GetRequestFromPmidNodeToDataManager& message,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver) {
  return GetFromCache(message, sender, receiver,
                      is_cacheable<nfs::DataName(message.Contents>()));
}


template<typename T>
bool PmidNodeService::GetFromCache(const T& message,
                                   const typename T::Sender& sender,
                                   const typename T::Receiver& receiver,
                                   IsCacheable) {
  return CacheGet<T>(message,
                     sender,
                     receiver,
                     is_long_term_cachable<nfs::DataName(message.Contents())>);
}

template<typename T>
bool PmidNodeService::GetFromCache(const T& /*message*/,
                                   const typename T::Sender& /*sender*/,
                                   const typename T::Receiver& /*receiver*/,
                                   IsNotCacheable) {
  return false;
}


template<typename T>
bool PmidNodeService::CacheGet(const T& message,
                               const typename T::Sender& sender,
                               const typename T::Receiver& receiver,
                               IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<nfs::Dataname(message.Contents())>::value,
                "This should only be called for short-term cacheable data types.");
  NonEmptyString data(mem_only_cache_.Get(message.Contents()));
  if (data.empty())
    return false;
  // TODO(Mahmoud): Must send the data to the requestor
}

template<typename T>
NonEmptyString PmidNodeService::CacheGet(const T& message,
                                         const typename T::Sender& sender,
                                         const typename T::Receiver& receiver,
                                         IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<nfs::Dataname(message.Contents())>::value,
                "This should only be called for long-term cacheable data types.");
  NonEmptyString data(cache_data_store_.Get(message.Contents()));
  if (data.empty())
    return false;
  // TODO(Mahmoud): Must send the data to the requestor
}

template<>
void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
  auto data_name(GetDataNameVariant(message.contents->data->name.type,
                                    message.contents->data->name.raw_name));
  auto result(boost::apply_visitor(cacheable_type_visitor(), data_name));
  if (result)
    CacheStore(message, data_name, IsLongTermCacheable());
  else
    CacheStore(message, data_name, IsShortTermCacheable());
}

template<typename T>
void PmidNodeService::CacheStore(const T& message, const DataNameVariant& data_name, IsShortTermCacheable) {
//  static_assert(is_short_term_cacheable<nfs::DataName(message.Contents.data.name)>::value,
//                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(data_name, message.contents->data->content);
}

template<typename T>
void PmidNodeService::CacheStore(const T& message, const DataNameVariant& data_name, IsLongTermCacheable) {
//  static_assert(is_long_term_cacheable<nfs::DataName(message.Contents.data.name)>::value,
//                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(data_name, message.contents->data->content);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
