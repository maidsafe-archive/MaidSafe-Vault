/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_ACCOUNT_TRANSFER_HANDLER_H_
#define MAIDSAFE_VAULT_ACCOUNT_TRANSFER_HANDLER_H_

#include <algorithm>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include "boost/optional.hpp"

#include "maidsafe/common/node_id.h"

#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {

template <typename AccountType>
class AccountTransferHandler {
 public:
  enum class AddResult {
    kSuccess,
    kWaiting,
    kFailure
  };

  struct Result {
    Result(const typename AccountType::Key& key_in,
           const boost::optional<typename AccountType::Value> value_in,
           const AddResult& result_in):
        key(key_in), value(*value_in), result(result_in) {}
    typename AccountType::Key key;
    boost::optional<typename AccountType::Value> value;
    AddResult result;
  };

  AccountTransferHandler();

  using KeyValuePair = std::pair<typename AccountType::Key, typename AccountType::Value>;

  Result Add(const KeyValuePair& account, const NodeId& source_id);

  AccountTransferHandler(const AccountTransferHandler&) = delete;
  AccountTransferHandler& operator=(const AccountTransferHandler&) = delete;
  AccountTransferHandler(AccountTransferHandler&&) = delete;
  AccountTransferHandler& operator=(AccountTransferHandler&&) = delete;

 private:
  using ValueType = std::pair<NodeId, typename AccountType::Value>;

  std::map<typename AccountType::Key, std::vector<ValueType>> kv_pairs_;
  mutable std::mutex mutex_;
};

// ==================== Implementation =============================================================

template <typename AccountType>
AccountTransferHandler<AccountType>::AccountTransferHandler()
    : kv_pairs_(), mutex_() {}

template <typename AccountType>
typename AccountTransferHandler<AccountType>::Result
AccountTransferHandler<AccountType>::Add(const KeyValuePair& account, const NodeId& source_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter(kv_pairs_.find(account.first));
  if (iter != std::end(kv_pairs_)) {
    // replace entry from existing sender
    iter->second.erase(std::remove_if(std::begin(iter->second), std::end(iter->second),
                                      [&](const std::pair<NodeId,
                                          typename AccountType::Value>& pair) {
                                        return pair.first == source_id;
                                      }), std::end(iter->second));
    iter->second.push_back(std::make_pair(source_id, account.second));
  } else {
    kv_pairs_.insert(
        std::make_pair(account.first,
                       std::vector<ValueType>{ std::make_pair(source_id, account.second) }));
  }
  auto resolution(AccountType::Resolve(iter->first, iter->second));
  if (resolution.result == AddResult::kSuccess) {
    kv_pairs_.erase(account.first);
    return Result(resolution.key, resolution.value, AddResult::kSuccess);
  } else if (resolution.result == AddResult::kFailure) {
    kv_pairs_.erase(account.first);
    return Result(account.first, boost::optional<typename AccountType::Value>(),
                  AddResult::kFailure);
  } else {
    return Result(account.first, boost::optional<typename AccountType::Value>(),
                  AddResult::kWaiting);
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_HANDLER_H_
