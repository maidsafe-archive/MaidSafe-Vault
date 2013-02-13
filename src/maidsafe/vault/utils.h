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

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/variant/static_visitor.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/data_message.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace detail {

inline routing::GroupRangeStatus NodeRangeCheck(routing::Routing& routing, const NodeId& node_id) {
  return routing.IsNodeIdInGroupRange(node_id);
}

bool ShouldRetry(routing::Routing& routing, const nfs::DataMessage& data_message);

MaidName GetSourceMaidName(const nfs::DataMessage& data_message);

// Ensure the mutex protecting accounts is locked throughout this call
template<typename Account>
typename std::vector<Account>::iterator FindAccount(
    std::vector<Account>& accounts,
    const typename Account::name_type& account_name) {
  return std::find_if(accounts.begin(),
                      accounts.end(),
                      [&account_name](const Account& account) {
                        return account_name == account.name();
                      });
}

// Ensure the mutex protecting accounts is locked throughout this call
template<typename Account>
typename std::vector<Account>::const_iterator FindAccount(
    const std::vector<Account>& accounts,
    const typename Account::name_type& account_name) {
  return std::find_if(accounts.begin(),
                      accounts.end(),
                      [&account_name](const Account& account) {
                        return account_name == account.name();
                      });
}

template<typename Account>
bool AddAccount(std::mutex& mutex, std::vector<Account>& accounts, const Account& account) {
  std::lock_guard<std::mutex> lock(mutex);
  if (FindAccount(accounts, account.name()) != accounts.end())
    return false;
  accounts.push_back(account);
  return true;
}

template<typename Account>
bool DeleteAccount(std::mutex& mutex,
                   std::vector<Account>& accounts,
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
    const std::vector<Account>& accounts,
    const typename Account::name_type& account_name) {
  std::lock_guard<std::mutex> lock(mutex);
  auto itr(FindAccount(accounts, account_name));
  if (itr == accounts.end())
    ThrowError(VaultErrors::no_such_account);

  return (*itr).Serialise();
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

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
