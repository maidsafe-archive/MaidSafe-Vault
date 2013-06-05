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

#ifndef MAIDSAFE_VAULT_UTILS_INL_H_
#define MAIDSAFE_VAULT_UTILS_INL_H_

#include <algorithm>
#include <cmath>
#include <vector>
#include <set>

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/accumulator.h"


namespace maidsafe {

namespace vault {

template<typename Message>
inline bool FromMaidAccountHolder(const Message& message) {
  return message.source().persona == nfs::Persona::kMaidAccountHolder;
}

template<typename Message>
inline bool FromMetadataManager(const Message& message) {
  return message.source().persona == nfs::Persona::kMetadataManager;
}

template<typename Message>
inline bool FromPmidAccountHolder(const Message& message) {
  return message.source().persona == nfs::Persona::kPmidAccountHolder;
}

template<typename Message>
inline bool FromDataHolder(const Message& message) {
  return message.source().persona == nfs::Persona::kDataHolder;
}

template<typename Message>
inline bool FromClientMaid(const Message& message) {
  return message.source().persona == nfs::Persona::kClientMaid;
}

template<typename Message>
inline bool FromClientMpid(const Message& message) {
  return message.source().persona == nfs::Persona::kClientMpid;
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
  return message.source().persona == nfs::Persona::kStructuredDataManager;
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
std::string ToFixedWidthString(int32_t number) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(number < std::pow(256, width) && number >= 0);
  std::string result(width, 0);
  for (int i(0); i != width; ++i) {
    result[width - i - 1] = static_cast<char>(number);
    number /= 256;
  }
  return result;
}

template<int width>
int32_t FromFixedWidthString(const std::string& number_as_string) {
  static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
  assert(static_cast<int>(number_as_string.size()) == width);
  int32_t result(0), factor(1);
  for (int i(0); i != width; ++i) {
    result += (number_as_string[i] * factor);
    factor *= 256;
  }
  assert(result < std::pow(256, width) && result >= 0);
  return result;
}

// Workaround for gcc 4.6 bug related to warning "redundant redeclaration" for template
// specialisation. refer // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867#c4
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

template<>
std::string ToFixedWidthString<1>(int32_t number);

template<>
int32_t FromFixedWidthString<1>(const std::string& number_as_string);

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_INL_H_
