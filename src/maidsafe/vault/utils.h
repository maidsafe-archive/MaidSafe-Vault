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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/common/error.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/utils.h"

#include "maidsafe/vault/disk_based_storage_pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

template<typename Message>
inline bool FromMaidAccountHolder(const Message& message) {
  return message.source().persona != nfs::Persona::kMaidAccountHolder;
}

template<typename Message>
inline bool FromMetadataManager(const Message& message) {
  return message.source().persona != nfs::Persona::kMetadataManager;
}

template<typename Message>
inline bool FromPmidAccountHolder(const Message& message) {
  return message.source().persona != nfs::Persona::kPmidAccountHolder;
}

template<typename Message>
inline bool FromDataHolder(const Message& message) {
  return message.source().persona != nfs::Persona::kDataHolder;
}

template<typename Message>
inline bool FromClientMaid(const Message& message) {
  return message.source().persona != nfs::Persona::kClientMaid;
}

template<typename Message>
inline bool FromClientMpid(const Message& message) {
  return message.source().persona != nfs::Persona::kClientMpid;
}

template<typename Message>
inline bool FromOwnerDirectoryManager(const Message& message) {
  return message.source().persona != nfs::Persona::kOwnerDirectoryManager;
}

template<typename Message>
inline bool FromGroupDirectoryManager(const Message& message) {
  return message.source().persona != nfs::Persona::kGroupDirectoryManager;
}

template<typename Message>
inline bool FromWorldDirectoryManager(const Message& message) {
  return message.source().persona != nfs::Persona::kWorldDirectoryManager;
}

template<typename Message>
inline bool FromDataGetter(const Message& message) {
  return message.source().persona != nfs::Persona::kDataGetter;
}


namespace detail {

bool ShouldRetry(routing::Routing& routing, const nfs::DataMessage& data_message);

MaidName GetSourceMaidName(const nfs::DataMessage& data_message);

template<typename Data>
bool IsDataElement(const typename Data::name_type& name,
                   const DataNameVariant& data_name_variant) {
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  auto type_and_name(boost::apply_visitor(type_and_name_visitor, data_name_variant));
  return name == type_and_name.second;
}

void SendReply(const nfs::DataMessage& original_message,
               const maidsafe_error& return_code,
               const routing::ReplyFunctor& reply_functor);

// Ensure the mutex protecting accounts is locked throughout this call
template<typename Account>
typename std::vector<std::unique_ptr<Account>>::iterator FindAccount(
    std::vector<std::unique_ptr<Account>>& accounts,
    const typename Account::name_type& account_name) {
  return std::find_if(accounts.begin(),
                      accounts.end(),
                      [&account_name](const std::unique_ptr<Account>& account) {
                        return account_name == account->name();
                      });
}

// Ensure the mutex protecting accounts is locked throughout this call
template<typename Account>
typename std::vector<std::unique_ptr<Account>>::const_iterator FindAccount(
    const std::vector<std::unique_ptr<Account>>& accounts,
    const typename Account::name_type& account_name) {
  return std::find_if(accounts.begin(),
                      accounts.end(),
                      [&account_name](const std::unique_ptr<Account>& account) {
                        return account_name == account->name();
                      });
}

template<typename Account>
bool AddAccount(std::mutex& mutex,
                std::vector<std::unique_ptr<Account>>& accounts,
                std::unique_ptr<Account>&& account) {
  std::lock_guard<std::mutex> lock(mutex);
  if (FindAccount(accounts, account->name()) != accounts.end())
    return false;
  accounts.push_back(std::move(account));
  return true;
}

template<typename Account>
bool DeleteAccount(std::mutex& mutex,
                   std::vector<std::unique_ptr<Account>>& accounts,
                   const typename Account::name_type& account_name) {
  std::lock_guard<std::mutex> lock(mutex);
  auto itr(FindAccount(accounts, account_name));
  if (itr != accounts.end())
    accounts.erase(itr);
  return true;
}

template<typename Account>
typename Account::serialised_type GetSerialisedAccount(
    std::mutex& mutex,
    const std::vector<std::unique_ptr<Account>>& accounts,
    const typename Account::name_type& account_name) {
  std::lock_guard<std::mutex> lock(mutex);
  auto itr(FindAccount(accounts, account_name));
  if (itr == accounts.end())
    ThrowError(VaultErrors::no_such_account);

  return (*itr)->Serialise();
}

template<typename Nfs, typename Data>
inline void RetryOnPutOrDeleteError(routing::Routing& routing,
                                    Nfs& nfs,
                                    nfs::DataMessage data_message) {
  if (ShouldRetry(routing, data_message)) {
    // TODO(Fraser#5#): 2013-01-24 - Replace this with repeating asio timer?  Incorporate larger
    //                  gaps between attempts.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    if (data_message.data().action == nfs::DataMessage::Action::kPut) {
      nfs.Put(data_message,
              [&routing, &nfs] (nfs::DataMessage data_msg) {
                RetryOnPutOrDeleteError<Nfs, Data>(routing, nfs, data_msg);
              });
    } else {
      assert(data_message.data().action == nfs::DataMessage::Action::kDelete);
      nfs.Delete(data_message,
                [&routing, &nfs] (nfs::DataMessage data_msg) {
                  RetryOnPutOrDeleteError<Nfs, Data>(routing, nfs, data_msg);
                });
    }
  }
}

// Returns true if the required successful request count has been reached
template<typename Accumulator>
bool AddResult(const nfs::DataMessage& data_message,
               const routing::ReplyFunctor& reply_functor,
               const maidsafe_error& return_code,
               Accumulator& accumulator,
               std::mutex& accumulator_mutex,
               int requests_required) {
  std::vector<typename Accumulator::PendingRequest> pending_requests;
  maidsafe_error overall_return_code(CommonErrors::success);
  const bool kDone(true);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex);
    auto pending_results(accumulator.PushSingleResult(data_message, reply_functor, return_code));
    if (static_cast<int>(pending_results.size()) < requests_required)
      return !kDone;

    auto result(nfs::GetSuccessOrMostFrequentReply(pending_results, requests_required));
    if (!result.second && pending_results.size() < routing::Parameters::node_group_size)
      return !kDone;

    overall_return_code = (*result.first).error();
    pending_requests = accumulator.SetHandled(data_message, overall_return_code);
  }

  for (auto& pending_request : pending_requests)
    SendReply(pending_request.msg, overall_return_code, pending_request.reply_functor);

  return kDone;
}


}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
