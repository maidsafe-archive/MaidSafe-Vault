/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_UTILS_INL_H_
#define MAIDSAFE_VAULT_UTILS_INL_H_

#include <algorithm>
#include <cmath>
#include <vector>
#include <set>
#include <string>

#include "maidsafe/routing/parameters.h"

//#include "maidsafe/vault/accumulator.h"


namespace maidsafe {

namespace vault {

//template<typename Message>
//inline bool FromMaidManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kMaidManager;
//}
//
//template<typename Message>
//inline bool FromDataManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kDataManager;
//}
//
//template<typename Message>
//inline bool FromPmidManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kPmidManager;
//}
//
//template<typename Message>
//inline bool FromDataHolder(const Message& message) {
//  return message.source().persona == nfs::Persona::kPmidNode;
//}
//
//template<typename Message>
//inline bool FromClientMaid(const Message& message) {
//  return message.source().persona == nfs::Persona::kMaidNode;
//}
//
//template<typename Message>
//inline bool FromClientMpid(const Message& message) {
//  return message.source().persona == nfs::Persona::kMpidNode;
//}
//
//template<typename Message>
//inline bool FromVersionManager(const Message& message) {
//  return message.source().persona == nfs::Persona::kVersionManager;
//}
//
//template<typename Message>
//inline bool FromDataGetter(const Message& message) {
//  return message.source().persona == nfs::Persona::kDataGetter;
//}
//
//template<typename Message>
//inline bool ValidateSyncSender(const nfs::Message& message) {
//  return message.source().persona == nfs::Persona::kVersionManager;
//}



namespace detail {

template<typename Data>
bool IsDataElement(const typename Data::Name& name,
                   const DataNameVariant& data_name_variant) {
  return DataNameVariant(name) == data_name_variant;
}

// Returns true if the required successful request count has been reached
//template<typename Accumulator>
//bool AddResult(const nfs::Message& message,
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
//    if (!result.second && pending_results.size() < routing::Parameters::node_group_size)
//      return false;

//    overall_return_code = (*result.first).error();
//    pending_requests = accumulator.SetHandled(message, nfs::Reply(overall_return_code));
//  }

//  for (auto& pending_request : pending_requests)
//    SendReply(pending_request.msg, overall_return_code, pending_request.reply_functor);

//  return true;
//}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_INL_H_
