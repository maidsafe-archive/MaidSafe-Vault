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
#include <exception>
#include <string>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/log.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/get_policies.h"
#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/delete_policies.h"
#include "maidsafe/vault/put_policies.h"

#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/data_store/data_store.h"

#include "maidsafe/vault/utils.h"


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
  bool IsInCache(const nfs::Message& message);
  template<typename Data>
  void StoreInCache(const nfs::Message& message);
  void StopSending();
  void ResumeSending();

  template<class T> friend class test::DataHolderTest;

 private:
  template<typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleGetMessage(nfs::Message message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePostMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

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

template<typename Data>
void DataHolder::HandleMessage(const nfs::Message& message,
                               const routing::ReplyFunctor& reply_functor) {
  LOG(kInfo) << "received message at Data holder";
  switch (message.action_type()) {
    case nfs::ActionType::kGet :
      HandleGetMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kPut :
      HandlePutMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kPost :
      HandlePostMessage<Data>(message, reply_functor);
      break;
    case nfs::ActionType::kDelete :
      HandleDeleteMessage<Data>(message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}

template<typename Data>
void DataHolder::HandleGetMessage(nfs::Message message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    message.set_content(permanent_data_store_.Get(typename Data::name_type(
                            Identity(message.destination()->node_id.string()))));
    reply_functor(message.Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandlePutMessage(const nfs::Message& message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    permanent_data_store_.Store(typename Data::name_type(
        Identity(message.destination()->node_id.string())), message.content());
    reply_functor(nfs::ReturnCode(0).Serialise()->string());
  } catch(std::exception& /*ex*/) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());  // non 0 plus optional message
    // error code // at the moment this will go back to client
    // in production it will g back to
  }
}

template<typename Data>
void DataHolder::HandlePostMessage(const nfs::Message& /*message*/,
                                   const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

template<typename Data>
void DataHolder::HandleDeleteMessage(const nfs::Message& message,
                                     const routing::ReplyFunctor& reply_functor) {
  permanent_data_store_.Delete(typename Data::name_type(
      Identity(message.destination()->node_id.string())));
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

// Cache Handling
template<typename Data>
bool DataHolder::IsInCache(const nfs::Message& message) {
  NonEmptyString result;
  try {
    if (is_long_term_cacheable<Data>::value) {
      result = cache_data_store_.Get(typename Data::name_type(
                   Identity(message.destination()->node_id.string())));
    } else {
      result = mem_only_cache_.Get(typename Data::name_type(
                   Identity(message.destination()->node_id.string())));
    }
    return (!result.string().empty());
  }
  catch(std::exception& error) {
    LOG(kInfo) << "data not cached on this node " << error.what();
    return false;
  }
}

template<typename Data>
void DataHolder::StoreInCache(const nfs::Message& message) {
  try {
    if (is_long_term_cacheable<Data>::value) {
      cache_data_store_.Store(typename Data::name_type(
          Identity(message.destination()->node_id.string())), message.content());
    } else {
      mem_only_cache_.Store(typename Data::name_type(
          Identity(message.destination()->node_id.string())), message.content());
    }
  }
  catch(std::exception& error) {
    LOG(kInfo) << "data could not be cached on this node " << error.what();
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_HOLDER_H_
