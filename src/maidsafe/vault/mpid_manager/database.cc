/*  Copyright 2015 MaidSafe.net limited

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

#include "maidsafe/vault/mpid_manager/database.h"

#include <utility>
#include <cstdint>
#include <string>

#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {

MpidManagerDatabase::MpidManagerDatabase() : container_(), mutex_() {}

void MpidManagerDatabase::Put(const MessageKey& key,
                              uint32_t size,
                              const GroupName& group_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByKey& key_index = boost::multi_index::get<EntryKey_Tag>(container_);
  auto iter(key_index.find(key));
  if (iter == std::end(key_index))
    container_.insert(DatabaseEntry(key, size, group_name));
  // just keep silent in case of double put
// else
//   BOOST_THROW_EXCEPTION(MakeError(VaultErrors::data_already_exists));
}

void MpidManagerDatabase::Delete(const MessageKey& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByKey& key_index = boost::multi_index::get<EntryKey_Tag>(container_);
  key_index.erase(key);
}

bool MpidManagerDatabase::Has(const MessageKey& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByKey& key_index = boost::multi_index::get<EntryKey_Tag>(container_);
  auto iter(key_index.find(key));
  return iter != std::end(key_index);
}

bool MpidManagerDatabase::HasGroup(const GroupName& mpid) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
  auto itr(mpid_index.lower_bound(mpid));
  if (itr != mpid_index.end())
    return itr->mpid == mpid;
  else
    return false;
}

MessageKey MpidManagerDatabase::GetAccountChunkName(const GroupName& mpid) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
  auto itr0(mpid_index.lower_bound(mpid));
  auto itr1(mpid_index.upper_bound(mpid));
  while (itr0 != itr1) {
    if (itr0->size == 0)
      return itr0->key;
    ++itr0;
  }
  BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
}

std::pair<uint32_t, uint32_t> MpidManagerDatabase::GetStatistic(const GroupName& mpid) {
  std::lock_guard<std::mutex> lock(mutex_);
  uint32_t num_of_messages(0), total_size(0);
  EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
  auto itr0(mpid_index.lower_bound(mpid));
  auto itr1(mpid_index.upper_bound(mpid));
  while (itr0 != itr1) {
    ++num_of_messages;
    total_size += itr0->size;
    ++itr0;
  }
  return std::make_pair(num_of_messages, total_size);
}

std::vector<MessageKey> MpidManagerDatabase::GetEntriesForMPID(const GroupName& mpid) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<MessageKey> entries;
  EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
  auto itr0(mpid_index.lower_bound(mpid));
  auto itr1(mpid_index.upper_bound(mpid));
  while (itr0 != itr1) {
    entries.push_back(itr0->key);
    ++itr0;
  }
  return entries;
}

// DbTransferInfo MpidManagerDatabase::GetTransferInfo(
//    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
//  std::vector<std::pair<NodeId, GroupName>> groups_to_be_transferred;
//  std::vector<GroupName> groups_to_be_removed;
//  {
//    std::lock_guard<std::mutex> lock(mutex_);
//    EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
//    auto it0 = std::begin(mpid_index);
//    while (it0 != std::end(mpid_index)) {
//      auto check_holder_result = close_nodes_change->CheckHolders(NodeId(it0->mpid->string()));
//      if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
//        if (check_holder_result.new_holder != NodeId())
//          groups_to_be_transferred.push_back(std::make_pair(check_holder_result.new_holder,
//                                                            it0->mpid));
//      } else {
//  //      VLOG(VisualiserAction::kRemoveAccount, key.name);
//        groups_to_be_removed.push_back(it0->mpid);
//        // empty NodeId indicates removing from local
//        groups_to_be_transferred.push_back(std::make_pair(NodeId(), it0->mpid));
//      }
//      it0 = mpid_index.upper_bound(it0->mpid);
//    }
//  }
//  DbTransferInfo transfer_info;
//  {
//    std::lock_guard<std::mutex> lock(mutex_);
//    EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
//    for (const auto& transfer_entry : groups_to_be_transferred) {
//      auto itr0(mpid_index.lower_bound(transfer_entry.second));
//      auto itr1(mpid_index.upper_bound(transfer_entry.second));
//      while (itr0 != itr1) {
//        PutIntoTransferInfo(transfer_entry.first, transfer_entry.second,
//                            itr0->key, transfer_info);
//        ++itr0;
//      }
//    }
//  }
//  for (const auto& group_name : groups_to_be_removed)
//    DeleteGroup(group_name);

//  return transfer_info;
// }

void MpidManagerDatabase::DeleteGroup(const GroupName& mpid) {
  std::lock_guard<std::mutex> lock(mutex_);
  EntryByMpid& mpid_index = boost::multi_index::get<EntryMpid_Tag>(container_);
  auto itr0(mpid_index.lower_bound(mpid));
  auto itr1(mpid_index.upper_bound(mpid));
  while (itr0 != itr1)
    itr0 = mpid_index.erase(itr0);
}

// void MpidManagerDatabase::PutIntoTransferInfo(const NodeId& new_holder,
//                                              const GroupName& mpid,
//                                              const MessageKey& key,
//                                              DbTransferInfo& transfer_info) {
//  auto found_itr = transfer_info.find(new_holder);
//  if (found_itr != transfer_info.end()) {  // append
//    found_itr->second.push_back(std::make_pair(mpid, key));
//  } else {  // create
//    std::vector<GKPair> group_key_vector;
//    group_key_vector.push_back(std::make_pair(mpid, key));
//    transfer_info.insert(std::make_pair(new_holder, std::move(group_key_vector)));
//  }
// }

}  // namespace vault

}  // namespace maidsafe
