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

#include "maidsafe/vault/accumulator.h"


namespace maidsafe {

namespace vault {

template<typename Message>
inline bool FromMaidManager(const Message& message) {
  return message.source().persona == nfs::Persona::kMaidManager;
}

template<typename Message>
inline bool FromDataManager(const Message& message) {
  return message.source().persona == nfs::Persona::kDataManager;
}

template<typename Message>
inline bool FromPmidManager(const Message& message) {
  return message.source().persona == nfs::Persona::kPmidManager;
}

template<typename Message>
inline bool FromDataHolder(const Message& message) {
  return message.source().persona == nfs::Persona::kPmidNode;
}

template<typename Message>
inline bool FromClientMaid(const Message& message) {
  return message.source().persona == nfs::Persona::kMaidNode;
}

template<typename Message>
inline bool FromClientMpid(const Message& message) {
  return message.source().persona == nfs::Persona::kMpidNode;
}

template<typename Message>
inline bool FromOwnerDirectoryManager(const Message& message) {
  return message.source().persona == nfs::Persona::kOwnerDirectoryManager;
}

template<typename Message>
inline bool FromGroupDirectoryManager(const Message& message) {
  return message.source().persona == nfs::Persona::kGroupDirectoryManager;
}

template<typename Message>
inline bool FromWorldDirectoryManager(const Message& message) {
  return message.source().persona == nfs::Persona::kWorldDirectoryManager;
}

template<typename Message>
inline bool FromDataGetter(const Message& message) {
  return message.source().persona == nfs::Persona::kDataGetter;
}

template<typename Message>
inline bool ValidateSyncSender(const nfs::Message& message) {
  return message.source().persona == nfs::Persona::kVersionManager;
}



namespace detail {

template<typename Data>
bool IsDataElement(const typename Data::name_type& name,
                   const DataNameVariant& data_name_variant) {
  return DataNameVariant(name) == data_name_variant;
}

// Returns true if the required successful request count has been reached
template<typename Accumulator>
bool AddResult(const nfs::Message& message,
               const routing::ReplyFunctor& reply_functor,
               const maidsafe_error& return_code,
               Accumulator& accumulator,
               std::mutex& accumulator_mutex,
               int requests_required) {
  std::vector<typename Accumulator::PendingRequest> pending_requests;
  maidsafe_error overall_return_code(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex);
    auto pending_results(accumulator.PushSingleResult(message, reply_functor,
                                                      nfs::Reply(return_code)));
    if (static_cast<int>(pending_results.size()) < requests_required)
      return false;

    auto result(nfs::GetSuccessOrMostFrequentReply(pending_results, requests_required));
    if (!result.second && pending_results.size() < routing::Parameters::node_group_size)
      return false;

    overall_return_code = (*result.first).error();
    pending_requests = accumulator.SetHandled(message, nfs::Reply(overall_return_code));
  }

  for (auto& pending_request : pending_requests)
    SendReply(pending_request.msg, overall_return_code, pending_request.reply_functor);

  return true;
}

template<int width>
std::string ToFixedWidthString(uint32_t number) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(number < std::pow(256, width));
  std::string result(width, 0);
  for (int i(0); i != width; ++i) {
    result[width - i - 1] = static_cast<char>(number);
    number /= 256;
  }
  return result;
}

template<int width>
uint32_t FromFixedWidthString(const std::string& number_as_string) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(static_cast<int>(number_as_string.size()) == width);
  uint32_t result(0), factor(1);
  for (int i(0); i != width; ++i) {
    result += (static_cast<unsigned char>(number_as_string[width - i - 1]) * factor);
    factor *= 256;
  }
  assert(result < std::pow(256, width));
  return result;
}

// Workaround for gcc 4.6 bug related to warning "redundant redeclaration" for template
// specialisation. refer // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867#c4
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

template<>
std::string ToFixedWidthString<1>(uint32_t number);

template<>
uint32_t FromFixedWidthString<1>(const std::string& number_as_string);

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_INL_H_
