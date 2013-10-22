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

#ifndef MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_VISITORS_H_
#define MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_VISITORS_H_

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/cache_handler/service.h"

namespace maidsafe {

namespace vault {

namespace detail {

namespace {
  typedef std::true_type IsCacheable;
  typedef std::false_type IsNotCacheable;
}

template<typename Sender>
class GetFromCacheVisitor : public boost::static_visitor<bool> {
 public:
  GetFromCacheVisitor(CacheHandlerService* cache_handler_service, const Sender& sender)
      : cache_handler_service_(cache_handler_service), kSender(sender) {}

  template <typename DataName>
  result_type operator()(const DataName& data_name) {
    auto cache_data(GetFromCache(data_name, is_cacheable<typename DataName::data_type>()));
    if (cache_data) {
      cache_handler_service_->template SendGetResponse(cache_data, kSender);
      return true;
    }
    return false;
  }

 private:
  template<typename DataName>
  boost::optional<typename DataName::data_type>
  GetFromCache(const DataName& data_name, IsCacheable) {
    return cache_handler_service_->template CacheGet<typename DataName::data_type>(
        data_name, is_long_term_cacheable<typename DataName::data_type>());
  }

  template<typename DataName>
  boost::optional<typename DataName::data_type>
  GetFromCache(const DataName& /*data_name*/, IsNotCacheable) {
    return boost::optional<typename DataName::data_type>();
  }

  CacheHandlerService* const cache_handler_service_;
  Sender kSender;
};

class PutToCacheVisitor : public boost::static_visitor<> {
 public:
  PutToCacheVisitor(CacheHandlerService* cache_handler_service, NonEmptyString content)
      : cache_handler_service_(cache_handler_service), kContent_(std::move(content)) {}

  template <typename DataName>
  void operator()(const DataName& data_name) {
    PutToCache(typename DataName::data_type(
                            data_name,
                            typename DataName::data_type::serialised_type(kContent_)),
               is_cacheable<typename DataName::data_type>());
  }

 private:
  template <typename Data>
  void PutToCache(const Data& data, IsCacheable) {
    cache_handler_service_->CacheStore(data, is_long_term_cacheable<Data>());
  }

  template <typename Data>
  void PutToCache(const Data& /*data_name*/, IsNotCacheable) {}

  CacheHandlerService* const cache_handler_service_;
  NonEmptyString kContent_;
};

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_OPERATION_VISITORS_H_
