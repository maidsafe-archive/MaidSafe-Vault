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

#include "maidsafe/vault/data_holder/data_holder_service.h"

#include <string>

#include "maidsafe/common/utils.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account_pb.h"


namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage / 5);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
//  boost::filesystem::space_info space = boost::filesystem::space("/tmp/vault_root_dir\\");  // FIXME  NOLINT

//  DiskUsage disk_total = DiskUsage(space.available);
//  DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//  DiskUsage cache_size = DiskUsage(disk_total * 0.1);

}  // unnamed namespace

DataHolder::DataHolder(const passport::Pmid& pmid,
                       routing::Routing& routing,
                       const boost::filesystem::path& vault_root_dir)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      cache_size_(disk_total_ / 10),
      permanent_data_store_(vault_root_dir / "data_holder" / "permanent", DiskUsage(10000)/*perm_usage*/),  // TODO(Fraser) BEFORE_RELEASE need to read value from disk
      cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                        vault_root_dir / "data_holder" / "cache"),  // FIXME - DiskUsage  NOLINT
      mem_only_cache_(mem_only_cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                      vault_root_dir / "data_holder" / "cache"),  // FIXME - DiskUsage should be 0  NOLINT
      stop_sending_(false),
      nfs_(routing, pmid),
      message_sequence_(),
      elements_to_store_() {
//  nfs_.GetElementList();  // TODO (Fraser) BEFORE_RELEASE Implementation needed
}

void DataHolder::HandleGenericMessage(const nfs::GenericMessage& /*generic_message*/,
                                      const routing::ReplyFunctor& /*reply_functor*/) {
  // verify the action type is GetElementList and is from PmidAccountHolder Persona
//  nfs::GenericMessage::Action action(generic_message.action());
//  std::vector<data_store::PermanentStore::KeyType> fetched_element_list;
//  if ((action == nfs::GenericMessage::Action::kGetElementList) &&
//      (generic_message.source().persona == nfs::Persona::kPmidAccountHolder)) {
//    // Two optional field in GenericMessage will be created :
//    //    message_index : current sequence num
//    //    message_count : how many messages to be expected
//    // expect multiple responses: once message_count reaches, any after-coming msg can be ignored
//    //                            when receiving a duplicating message_index, just ignore the msg
//    if (message_sequence_.size() < generic_message.message_count()) {
//      auto itr = std::find(message_sequence_.begin(), message_sequence_.end(),
//                           generic_message.message_index);
//      if (itr == message_sequence_.end()) {
//        protobuf::ArchivedPmidData archived_pmid_data;
//        archived_pmid_data.ParseFromString(generic_message.content().string());
//        for (auto& data : archived_pmid_data.data_stored())
//          fetched_element_list.push_back(data_store::PermanentStore::KeyType(data.name()));
//        // Will avoid the duplicates ?
//        elements_to_store_.insert(fetched_element_list.begin(), fetched_element_list.end());
//        message_sequence_.insert(generic_message.message_index);

//        if (message_sequence_.size() == generic_message.message_count()) {
//          // End of response
//          std::async([this, elements_to_store_] {
//              std::vector<data_store::PermanentStore::KeyType> element_to_fetch =
//                  this->permanent_data_store_.ElementsToStore(elements_to_store);
//              this->FetchElement(element_to_fetch);
//              elements_to_store_.clear();
//          });
//        }
//      }
//    }
//  }
}

//void DataHolder::FetchElement(const std::vector<data_store::PermanentStore::KeyType>& elements) {
//  std::vector<std::future<void>> future_gets;
//  for (auto& element : elements) {
//    future_gets.push_back(std::async([this, element] {
//          auto fetched_data = this->nfs_.Get(element);
//          try {
//            auto fetched_content = this->fetched_data.get();
//            this->permanent_data_store_.Put(element, fetched_content);
//          } catch(const std::exception& e) {
//            // no op
//          }
//        }));
//  }

//  for (auto& future_get : future_gets) {
//    try {
//      future_get.get();
//    }
//    catch(const std::exception& e) {
//      std::string msg(e.what());
//      LOG(kError) << msg;
//    }
//  }
//}

DataHolder::~DataHolder() {}

void DataHolder::CloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {}

}  // namespace vault

}  // namespace maidsafe
