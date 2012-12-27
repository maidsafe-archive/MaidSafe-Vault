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

#include "maidsafe/vault/data_holder.h"

#include "boost/filesystem/path.hpp"
#include "boost/filesystem.hpp"

#include "maidsafe/common/utils.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"

namespace maidsafe {

namespace vault {
namespace {
MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage * 0.2);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 0.4);
MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 0.4);
//boost::filesystem::space_info space = boost::filesystem::space("vault_root_dir");  // FIXME

//DiskUsage disk_total = DiskUsage(space.available);
//DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//DiskUsage cache_size = DiskUsage(disk_total * 0.1);
}

DataHolder::DataHolder(const boost::filesystem::path& vault_root_dir)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 0.8),
      cache_size_(disk_total_ * 0.1),
      persona_dir_(vault_root_dir / "data_holder"),
      persona_dir_permanent_(persona_dir_ / "permanent"),
      persona_dir_cache_(persona_dir_ / "cache"),
      permanent_data_store_(perm_usage, permanent_size_, nullptr, persona_dir_permanent_),
      cache_data_store_(cache_usage, cache_size_, nullptr, persona_dir_cache_),
      mem_only_cache_(mem_only_cache_usage, DiskUsage(0), nullptr, persona_dir_cache_),  //FIXME
      stop_sending_(false)
      {
        boost::filesystem::exists(persona_dir_) ||
            boost::filesystem::create_directory(persona_dir_);
        boost::filesystem::exists(persona_dir_permanent_) ||
            boost::filesystem::create_directory(persona_dir_permanent_);
        boost::filesystem::exists(persona_dir_cache_) ||
            boost::filesystem::create_directory(persona_dir_cache_);
      }

DataHolder::~DataHolder() {
}

void DataHolder::HandleMessage(const nfs::Message& message,
                               const routing::ReplyFunctor& reply_functor) {
  switch (message.action_type()) {
    case nfs::ActionType::kGet :
      HandleGetMessage(message, reply_functor);
      break;
    case nfs::ActionType::kPut :
      HandlePutMessage(message, reply_functor);
      break;
    case nfs::ActionType::kPost :
      HandlePostMessage(message, reply_functor);
      break;
    case nfs::ActionType::kDelete :
      HandleDeleteMessage(message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}
//need to fill in reply functors
// also in real system check msg.src came from a close node
void DataHolder::HandlePutMessage(const nfs::Message& /*message*/,
                                  const routing::ReplyFunctor& /*reply_functor*/) {
//  permanent_data_store_.Store(message.data_type(), message.content().name());
}

void DataHolder::HandleGetMessage(const nfs::Message& message,
                                  const routing::ReplyFunctor& reply_functor) {
  TaggedValue<Identity, passport::detail::AnsmidTag> key;
  key.data = Identity(message.content().string());
  auto result(permanent_data_store_.Get(key));
  reply_functor(result.string());
}

void DataHolder::HandlePostMessage(const nfs::Message& /*message*/,
                                   const routing::ReplyFunctor& /*reply_functor*/) {
// no op
}

void DataHolder::HandleDeleteMessage(const nfs::Message& /*message*/,
                                     const routing::ReplyFunctor& /*reply_functor*/) {
//  permenent_data_store.Delete(message.data_type() message.content().name());
}

// Cache Handling

bool DataHolder::HaveCache(nfs::Message& /*message*/) {
  try {
//    message.set_data(cache_data_store.Get(message.data_type() message.content().name());
    return true;
  }
  catch (std::exception& error) {
    LOG(kInfo) << "data not cached on this node " << error.what();
    return false;
  }
}

void DataHolder::StoreCache(const nfs::Message& /*message*/) {
  try {
//    cache_data_store.Store(message.data_type() message.content());
  }
  catch (std::exception& error) {
  LOG(kInfo) << "data could not be cached on this node " << error.what();
  }
}

void DataHolder::ResumeSending() {
  stop_sending_ = false;
}

void DataHolder::StopSending() {
  stop_sending_ = true;
}

}  // namespace vault

}  // namespace maidsafe
