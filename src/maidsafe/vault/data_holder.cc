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

#include "maidsafe/common/utils.h"
#include "maidsafe/common/types.h"

#include "maidsafe/data_store/data_buffer.h"

namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage / 5);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
//boost::filesystem::space_info space = boost::filesystem::space("vault_root_dir");  // FIXME

//DiskUsage disk_total = DiskUsage(space.available);
//DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//DiskUsage cache_size = DiskUsage(disk_total * 0.1);

}  // unnamed namespace

DataHolder::DataHolder(const boost::filesystem::path& vault_root_dir)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      cache_size_(disk_total_ / 10),
      persona_dir_(vault_root_dir / "data_holder"),
      persona_dir_permanent_(persona_dir_ / "permanent"),
      persona_dir_cache_(persona_dir_ / "cache"),
      permanent_data_store_(perm_usage, permanent_size_, nullptr, persona_dir_permanent_),
      cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr, persona_dir_cache_),  //FIXME - DiskUsage
      mem_only_cache_(mem_only_cache_usage, DiskUsage(cache_size_ / 2), nullptr, persona_dir_cache_),  //FIXME - DiskUsage should be 0
      stop_sending_(false) {
  boost::filesystem::exists(persona_dir_) ||
      boost::filesystem::create_directory(persona_dir_);
  boost::filesystem::exists(persona_dir_permanent_) ||
      boost::filesystem::create_directory(persona_dir_permanent_);
  boost::filesystem::exists(persona_dir_cache_) ||
      boost::filesystem::create_directory(persona_dir_cache_);
}

DataHolder::~DataHolder() {}

void DataHolder::ResumeSending() {
  stop_sending_ = false;
}

void DataHolder::StopSending() {
  stop_sending_ = true;
}

}  // namespace vault

}  // namespace maidsafe
