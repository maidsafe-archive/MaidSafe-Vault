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

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void PmidNodeService::HandleMessage(const nfs::Message& message,
                                      const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  switch (message.data().action) {
    case nfs::MessageAction::kPut:
      return HandlePutMessage<Data>(message, reply_functor);
    case nfs::MessageAction::kGet:
      return HandleGetMessage<Data>(message, reply_functor);
    case nfs::MessageAction::kDelete:
      return HandleDeleteMessage<Data>(message, reply_functor);
    case nfs::MessageAction::kAccountTransfer:
      return HandleAccountTransfer<Data>(message, reply_functor);
    default: {
      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
      std::lock_guard<std::mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, reply);
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<typename Data>
void PmidNodeService::HandlePutMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidatePutSender(message);
#endif
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired)) {
      permanent_data_store_.Put(data.name(), message.data().content);
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kPutRequestsRequired);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kPutRequestsRequired);
  }
}

template<typename Data>
void PmidNodeService::HandleGetMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidateGetSender(message);
#endif
    typename Data::name_type data_name(message.data().name);
    nfs::Reply reply(CommonErrors::success, permanent_data_store_.Get(data_name));
    reply_functor(reply.Serialise()->string());
  } catch(const std::exception& /*ex*/) {
    reply_functor(nfs::Reply(CommonErrors::unknown,
                             message.Serialise().data).Serialise()->string());
  }
}

template<typename Data>
void PmidNodeService::HandleDeleteMessage(const nfs::Message& message,
                                          const routing::ReplyFunctor& reply_functor) {
  try {
#ifndef TESTING
    ValidateDeleteSender(message);
#endif
    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired)) {
      permanent_data_store_.Delete(typename Data::name_type(message.data().name));
    }
  }
  catch(const maidsafe_error& error) {
    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
                      kDeleteRequestsRequired);
  }
  catch(...) {
    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired);
  }
}

template<typename Data>
void PmidNodeService::HandleAccountTransfer(const nfs::Message& message,
                                            const routing::ReplyFunctor& reply_functor) {
  bool send_account_transfer(false);
  std::map<DataNameVariant, u_int16_t> expected_chunks;
  protobuf::PmidAccountResponse pmid_account_response;
  pmid_account_response.ParseFromString(message.data().content.string());
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    uint16_t total_replies(TotalPmidAccountReplies());
    assert((total_replies < routing::Parameters::node_group_size) &&
           "Invalid number of account transfer");
    if (total_replies < routing::Parameters::node_group_size / 2) {
      accumulator_.pending_requests_.push_back(
          Accumulator<DataNameVariant>::PendingRequest(
              message, reply_functor, nfs::Reply(maidsafe_error(CommonErrors::success))));
      return;
    } else {
      uint16_t total_valid_replies(TotalValidPmidAccountReplies());
      total_replies++;  // to address the current response
      if (pmid_account_response.status() == static_cast<int>(CommonErrors::success))
        total_valid_replies++;
      if ((total_replies >= (routing::Parameters::node_group_size / 2 + 1)) &&
           total_valid_replies >= routing::Parameters::node_group_size) {
        ApplyAccountTransfer(total_replies, total_valid_replies, expected_chunks);
      } else if ((total_replies == routing::Parameters::node_group_size) ||
                 (total_valid_replies == routing::Parameters::node_group_size - 1)) {
        send_account_transfer = true;
        accumulator_.pending_requests_.erase(
            std::remove_if(accumulator_.pending_requests_.begin(),
                           accumulator_.pending_requests_.end(),
                           [](const Accumulator<DataNameVariant>::PendingRequest& pending_request) {
                             return pending_request.msg.data().action ==
                                        nfs::MessageAction::kAccountTransfer;
                           }), accumulator_.pending_requests_.end());
      } else {
        Accumulator<DataNameVariant>::PendingRequest
            pending_request(message,
                            reply_functor,
                            nfs::Reply(maidsafe_error(CommonErrors::success)));
        accumulator_.pending_requests_.push_back(pending_request);
      }
    }
  }
  if (send_account_transfer)
    SendAccountRequest();
  else
    UpdateLocalStorage(expected_chunks);
}

template<typename Data>
NonEmptyString PmidNodeService::GetFromCache(const nfs::Message& message) {
  return GetFromCache<Data>(message, is_cacheable<Data>());
}

template<typename Data>
NonEmptyString PmidNodeService::GetFromCache(const nfs::Message& message, IsCacheable) {
  return CacheGet<Data>(typename Data::name_type(message.data().name),
                        is_long_term_cacheable<Data>());
}

template<typename Data>
NonEmptyString PmidNodeService::GetFromCache(const nfs::Message& /*message*/, IsNotCacheable) {
  return NonEmptyString();
}

template<typename Data>
NonEmptyString PmidNodeService::CacheGet(const typename Data::name_type& name,
                                         IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Get(name);
}

template<typename Data>
NonEmptyString PmidNodeService::CacheGet(const typename Data::name_type& name,
                                         IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Get(name);
}

template<typename Data>
void PmidNodeService::StoreInCache(const nfs::Message& message) {
  StoreInCache<Data>(message, is_cacheable<Data>());
}

template<typename Data>
void PmidNodeService::StoreInCache(const nfs::Message& message, IsCacheable) {
  CacheStore<Data>(typename Data::name_type(message.data().name), message.data().content,
                   is_long_term_cacheable<Data>());
}

template<typename Data>
void PmidNodeService::StoreInCache(const nfs::Message& /*message*/, IsNotCacheable) {}

template<typename Data>
void PmidNodeService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<Data>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(name, value);
}

template<typename Data>
void PmidNodeService::CacheStore(const typename Data::name_type& name,
                            const NonEmptyString& value,
                            IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<Data>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(name, value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
