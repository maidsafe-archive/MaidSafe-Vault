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

#ifndef MAIDSAFE_VAULT_UTILS_INL_H_
#define MAIDSAFE_VAULT_UTILS_INL_H_

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <utility>

#include "maidsafe/routing/parameters.h"

// #include "maidsafe/vault/accumulator.h"

namespace maidsafe {

namespace vault {

// template<typename Message>
// inline bool FromMaidManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kMaidManager;
// }
//
// template<typename Message>
// inline bool FromDataManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kDataManager;
// }
//
// template<typename Message>
// inline bool FromPmidManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kPmidManager;
// }
//
// template<typename Message>
// inline bool FromDataHolder(const Message& message) {
//  return message.source().persona == nfs::Persona::kPmidNode;
// }
//
// template<typename Message>
// inline bool FromClientMaid(const Message& message) {
//  return message.source().persona == nfs::Persona::kMaidNode;
// }
//
// template<typename Message>
// inline bool FromClientMpid(const Message& message) {
//  return message.source().persona == nfs::Persona::kMpidNode;
// }
//
// template<typename Message>
// inline bool FromVersionHandler(const Message& message) {
//  return message.source().persona == nfs::Persona::kVersionHandler;
// }
//
// template<typename Message>
// inline bool FromDataGetter(const Message& message) {
//  return message.source().persona == nfs::Persona::kDataGetter;
// }
//
// template<typename Message>
// inline bool ValidateSyncSender(const nfs::Message& message) {
//  return message.source().persona == nfs::Persona::kVersionHandler;
// }

namespace detail {

// option 1 : Fire functor here with check_holder_result.new_holder & the corresponding value
// option 2 : create a map<NodeId, std::vector<std::pair<Key, value>>> and return after pruning
template <typename Key, typename Value, typename TransferInfo>
TransferInfo GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change,
    std::map<Key, Value>& accounts) {
  std::vector<Key> prune_vector;
  TransferInfo transfer_info;
  LOG(kVerbose) << "GetTransferInfo";
  for (auto& account : accounts) {
    auto check_holder_result = close_nodes_change->CheckHolders(
          NodeId(account.first->string()));
    if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
      LOG(kVerbose) << "GetTransferInfo in range";
      if (check_holder_result.new_holder == NodeId())
        continue;
      LOG(kVerbose) << "GetTransferInfo having new holder "
                    << check_holder_result.new_holder;
      auto found_itr = transfer_info.find(check_holder_result.new_holder);
      if (found_itr != transfer_info.end()) {
        LOG(kInfo) << "GetTransferInfo add into transfering account "
                   << HexSubstr(account.first->string())
                   << " to " << check_holder_result.new_holder;
        found_itr->second.push_back(account);
      } else {  // create
        LOG(kInfo) << "GetTransferInfo create transfering account "
                   << HexSubstr(account.first->string())
                   << " to " << check_holder_result.new_holder;
        std::vector<std::pair<Key, Value>> kv_pair;
        kv_pair.push_back(account);
        transfer_info.insert(std::make_pair(check_holder_result.new_holder, std::move(kv_pair)));
      }
    } else {
//      VLOG(VisualiserAction::kRemoveAccount, account.first.name);
      prune_vector.push_back(account.first);
    }
  }

  for (const auto& key : prune_vector)
    accounts.erase(key);
  return transfer_info;
}

template <typename Data>
bool IsDataElement(const typename Data::Name& name, const DataNameVariant& data_name_variant) {
  return DataNameVariant(name) == data_name_variant;
}

// Returns true if the required successful request count has been reached
// template<typename Accumulator>
// bool AddResult(const nfs::Message& message,
//               const routing::ReplyFunctor& reply_functor,
//               const maidsafe_error& return_code,
//               Accumulator& accumulator,
//               std::mutex& accumulator_mutex,
//               int requests_required) {
//  std::vector<typename Accumulator::PendingRequest> pending_requests;
//  maidsafe_error overall_return_code(CommonErrors::success);
//  {
//    std::lock_guard<std::mutex> lock(accumulator_mutex);
//    auto pending_results(accumulator.PushSingleResult(message, reply_functor,
//                                                      nfs::Reply(return_code)));
//    if (static_cast<int>(pending_results.size()) < requests_required)
//      return false;

//    auto result(nfs::GetSuccessOrMostFrequentReply(pending_results, requests_required));
//    if (!result.second && pending_results.size() < routing::Parameters::group_size)
//      return false;

//    overall_return_code = (*result.first).error();
//    pending_requests = accumulator.SetHandled(message, nfs::Reply(overall_return_code));
//  }

//  for (auto& pending_request : pending_requests)
//    SendReply(pending_request.msg, overall_return_code, pending_request.reply_functor);

//  return true;
// }

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_INL_H_
