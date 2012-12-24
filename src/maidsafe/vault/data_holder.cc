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
#include "boost/filesystem/filesystem.hpp"
#include "maidsafe/nfs/message.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"

namespace maidsafe {

namespace vault {
namespace {
MemoryUsage mem_usage = 524288000;  // 500Mb
MemoryUsage perm_usage = mem_usage * 0.2;
MemoryUsage cache_usage = mem_usage * 0.4;
MemoryUsage mem_only_cache_usage = mem_usage * 0.4;
boost::filesystem::space_info space(vault_root_dir);
DiskUsage disk_total = space.available();
DiskUsage permenent_size = disk_total * 0.8;
DiskUsage cache_size = disk_total * 0.1;
}

DataHolder::DataHolder(routing::Routing& routing, const boost::filesystem::path vault_root_dir)
    : persona_dir_(vault_root_dir + "data_holder"),
      persona_dir_permenent_(persona_dir_ + "permenent"),
      persona_dir_cache_(persona_dir_ + "cache"),
      permenent_data_store_(perm_usage, permement_size, std::nullptr_t, permenent_size),
      cache_data_store_(cache_usage, cache_size, [] {}, cache_size),
      mem_only_cache_(mem_only_cache_usage, 0, [] {}, cache_size),
      stop_sending_(false)
      {
        boost::filesystem::exists(persona_dir_) ||
            boost::filesystem::create_directory(persona_dir_);
        boost::filesystem::exists(persona_dir_permement_) ||
            boost::filesystem::create_directory(persona_dir_permenent_);
        boost::filesystem::exists(persona_dir_cache_) ||
            boost::filesystem::create_directory(persona_dir_cache_);
      }

void DataHolder::HandleMessage(const nfs::Message& message,
                               routing::ReplyFunctor reply_functor) {
  switch (message.action_type()) {
    case nfs::ActionType::kGet :
      HandleGetMessage(message);
      break;
    case nfs::ActionType::kPut :
      HandlePutMessage(message);
      break;
    case nfs::ActionType::kPost :
      HandlePostMessage(message);
      break;
    case nfs::ActionType::kDelete :
      HandleDeleteMessage(message);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}
//need to fill in reply functors
// also in real system check msg.src came from a close node
void DataHolder::HandlePutMessage(const nfs::Message& message,
                                  routing::ReplyFunctor reply_functor) {
  permenent_data_store.Store(message.data_type() message.content().name());
}

void DataHolder::HandleGetMessage(const nfs::Message& message,
                                  routing::ReplyFunctor reply_functor) {
  message.set_data(cache_data_store.Get(message.data_type() message.content().name());
}

void DataHolder::HandlePostMessage(const nfs::Message& message,
                                   routing::ReplyFunctor reply_functor) {
// no op
}

void DataHolder::HandleDeleteMessage(const nfs::Message& message,
                                     routing::ReplyFunctor reply_functor) {
  permenent_data_store.Delete(message.data_type() message.content().name());
}

// Cache Handling

bool DataHolder::HaveCache(nfs::Message& message) {
  try {
    message.set_data(cache_data_store.Get(message.data_type() message.content().name());
    return true;
  }
  catch (std::exception& error) {
    LOG(kInfo) << "data not cached on this node " << error.what();
    return false;
  }
}

void DataHolder::StoreCache(const nfs::Message& message) {
  try {
    cache_data_store.Store(message.data_type() message.content());
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
