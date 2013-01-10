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

#ifndef MAIDSAFE_VAULT_DATA_HOLDER_H_
#define MAIDSAFE_VAULT_DATA_HOLDER_H_

#include <atomic>
#include <type_traits>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/data_store/data_store.h"


namespace maidsafe {

namespace vault {

namespace test { template<class T> class DataHolderTest; }

class DataHolder {
 public:
  explicit DataHolder(const boost::filesystem::path& vault_root_dir);
  ~DataHolder();

  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  NonEmptyString GetFromCache(const nfs::Message& message);
  template<typename Data>
  void StoreInCache(const nfs::Message& message);

  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);

  void StopSending();
  void ResumeSending();

  template<class T> friend class test::DataHolderTest;

 private:
  template<typename Data>
  void HandleGetMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePostMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  // For short-term cacheable types
  template<typename Data>
  NonEmptyString CacheGet(const typename Data::name_type& name, std::false_type);
  // For long-term cacheable types
  template<typename Data>
  NonEmptyString CacheGet(const typename Data::name_type& name, std::true_type);
  // For short-term cacheable types
  template<typename Data>
  void CacheStore(const typename Data::name_type& name,
                  const NonEmptyString& value,
                  std::false_type);
  // For long-term cacheable types
  template<typename Data>
  void CacheStore(const typename Data::name_type& name,
                  const NonEmptyString& value,
                  std::true_type);

  boost::filesystem::space_info space_info_;
  DiskUsage disk_total_;
  DiskUsage permanent_size_;
  DiskUsage cache_size_;
  boost::filesystem::path persona_dir_;
  boost::filesystem::path persona_dir_permanent_;
  boost::filesystem::path persona_dir_cache_;
  data_store::DataStore<data_store::DataBuffer> permanent_data_store_;
  data_store::DataStore<data_store::DataBuffer> cache_data_store_;
  data_store::DataStore<data_store::DataBuffer> mem_only_cache_;
  std::atomic<bool> stop_sending_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/data_holder-inl.h"

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_H_
