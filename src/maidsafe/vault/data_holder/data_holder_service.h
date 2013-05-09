/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_H_

#include <mutex>
#include <type_traits>
#include <set>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/data_store/data_store.h"
#include "maidsafe/data_store/permanent_store.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/message.h"


#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace test {

template<typename Data>
class DataHolderTest;

}  // namespace test


class DataHolderService {
 public:
  DataHolderService(const passport::Pmid& pmid,
                    routing::Routing& routing,
                    const boost::filesystem::path& vault_root_dir);

  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  NonEmptyString GetFromCache(const nfs::Message& message);
  template<typename Data>
  void StoreInCache(const nfs::Message& message);

  template<typename Data>
  friend class test::DataHolderTest;

 private:
  typedef std::true_type IsCacheable, IsLongTermCacheable;
  typedef std::false_type IsNotCacheable, IsShortTermCacheable;

  template<typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleGetMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

  void ValidatePutSender(const nfs::Message& message) const;
  void ValidateGetSender(const nfs::Message& message) const;
  void ValidateDeleteSender(const nfs::Message& message) const;

  template<typename Data>
  NonEmptyString GetFromCache(const nfs::Message& message, IsCacheable);
  template<typename Data>
  NonEmptyString GetFromCache(const nfs::Message& message, IsNotCacheable);
  template<typename Data>
  NonEmptyString CacheGet(const typename Data::name_type& name, IsShortTermCacheable);
  template<typename Data>
  NonEmptyString CacheGet(const typename Data::name_type& name, IsLongTermCacheable);

  template<typename Data>
  void StoreInCache(const nfs::Message& message, IsCacheable);
  template<typename Data>
  void StoreInCache(const nfs::Message& message, IsNotCacheable);
  template<typename Data>
  void CacheStore(const typename Data::name_type& name,
                  const NonEmptyString& value,
                  IsShortTermCacheable);
  template<typename Data>
  void CacheStore(const typename Data::name_type& name,
                  const NonEmptyString& value,
                  IsLongTermCacheable);

  boost::filesystem::space_info space_info_;
  DiskUsage disk_total_;
  DiskUsage permanent_size_;
  DiskUsage cache_size_;
  data_store::PermanentStore permanent_data_store_;
  data_store::DataStore<data_store::DataBuffer> cache_data_store_;
  data_store::DataStore<data_store::DataBuffer> mem_only_cache_;
  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  Accumulator<DataNameVariant> accumulator_;
  DataHolderNfs nfs_;
  static const int kPutRequestsRequired_;
  static const int kDeleteRequestsRequired_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/data_holder/data_holder_service-inl.h"

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_DATA_HOLDER_SERVICE_H_
