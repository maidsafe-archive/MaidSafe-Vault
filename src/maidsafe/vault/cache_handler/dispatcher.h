/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_CACHE_HANDLER_DISPATCHER_H_
#define MAIDSAFE_VAULT_CACHE_HANDLER_DISPATCHER_H_

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/messages.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace detail {

template <typename RequestorIdType>
struct GetCacheResponseMessage {};

template <>
struct GetCacheResponseMessage<Requestor<nfs::SourcePersona<nfs::Persona::kMpidNode>>> {
  typedef nfs::GetCachedResponseFromCacheHandlerToMpidNode Type;
};

template <>
struct GetCacheResponseMessage<Requestor<nfs::SourcePersona<nfs::Persona::kDataGetter>>> {
  typedef nfs::GetCachedResponseFromCacheHandlerToDataGetter Type;
};

}  // namespace detail


class CacheHandlerDispatcher {
 public:
  explicit CacheHandlerDispatcher(routing::Routing& routing);

  template <typename Data, typename RequestorType>
  void SendGetResponse(const Data& data, const nfs::MessageId message_id,
                       const RequestorType& requestor);

 private:
  CacheHandlerDispatcher();
  CacheHandlerDispatcher(const CacheHandlerDispatcher&);
  CacheHandlerDispatcher(CacheHandlerDispatcher&&);
  CacheHandlerDispatcher& operator=(CacheHandlerDispatcher);

  routing::Routing& routing_;
};

template <typename Data, typename RequestorType>
void CacheHandlerDispatcher::SendGetResponse(const Data& data, const nfs::MessageId message_id,
                                             const RequestorType& requestor) {
  LOG(kVerbose) << "CacheHandlerDispatcher::SendGetResponse " << message_id;
  typedef typename detail::GetCacheResponseMessage<RequestorType>::Type Message;
  typedef routing::Message<typename Message::Sender, typename Message::Receiver> RoutingMessage;

  Message message(message_id, typename Message::Contents(data));
  RoutingMessage routing_message(message.Serialise(), routing::SingleSource(routing_.kNodeId()),
                                 routing::SingleId(requestor.node_id), routing::Cacheable::kPut);
  routing_.Send(routing_message);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_DISPATCHER_H_
