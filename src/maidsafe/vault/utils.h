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
#include <vector>

#include "maidsafe/common/error.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"

namespace maidsafe {

namespace vault {

namespace detail {

inline bool NodeRangeCheck(routing::Routing& routing, const NodeId& node_id) {
  return routing.IsNodeIdInGroupRange(node_id);
}

void ExtractElementsFromFilename(const std::string& filename,
                                 std::string& hash,
                                 size_t& file_number);

boost::filesystem::path GetFilePath(const boost::filesystem::path& base_path,
                                    const std::string& hash,
                                    size_t file_number);

bool MatchingDiskElements(const protobuf::DiskStoredElement& lhs,
                          const protobuf::DiskStoredElement& rhs);

// Ensure the mutex protecting accounts is locked throughout this call
template<typename Account>
std::vector<Account>::iterator FindAccount(std::vector<Account>& accounts,
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
  if (itr == accounts.end())
    return false;
  accounts.erase(itr);
  return true;
}

template<typename Account>
Account GetAccount(std::mutex& mutex,
                   std::vector<Account>& accounts,
                   const typename Account::name_type& account_name) {
  std::lock_guard<std::mutex> lock(mutex);
  auto itr(FindAccount(accounts, account_name));
  if (itr == accounts.end())
    ThrowError(CommonErrors::no_such_element);

  return *itr;
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
