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

#ifndef MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "maidsafe/common/node_id.h"

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/unresolved_account_transfer_action.h"

namespace maidsafe {

namespace vault {

template <typename AccountType>
class AccountTransferHandler {
 public:
  enum class AddResult {
    kSuccess,
    kWaiting,
    kFailure,
    kHandled
  };

  template <typename AccountType>
  struct Result {
    Result(const AccountType::Key& key_in,
           const boost::optional<AccountType::Value>& value_in, const AddResult& result_in):
        key(key_in), value(value_in), result(result_in) {}
    AccountType::Key key;
    boost::optional<AccountType::Value> value;
    AddResult result;
  };

  AccountTransferHandler();

  Result AddAccountTransferHandler(
      const std::pair<AccountType::Key, AccountType::Value>& request,
      const routing::GroupSource& source);

  Result Resolve(const AccountType::Key& key, const std::vector<AccountType::Value>& values);

 private:
  AccountTransferHandler(const AccountTransferHandler&);
  AccountTransferHandler& operator=(const AccountTransferHandler&);
  AccountTransferHandler(AccountTransferHandler&&);
  AccountTransferHandler& operator=(AccountTransferHandler&&);


  std::map<AccountType::Key, std::vector<std::pair<NodeId, AccountType::Value>>> kv_pairs_;
  mutable std::mutex mutex_;
};

// ==================== Implementation =============================================================

template <typename AccountType>
AccountTransferHandler<AccountType>::AccountTransferHandler()
    : kv_pairs_(),  mutex_() {}

template <typename AccountType>
AccountTransferHandler<AccountType>::Result
AccountTransferHandler<AccountType>::Add(
    const std::pair<AccountType::Key, AccountType::Value>& request,
    const routing::GroupSource& source) {
  std::lock_guard<std::mutex> lock;
  auto iter(kv_pairs_.find(request.first));
  if (iter != std::end(kv_pairs)) {
    // replace entry from existing sender
    iter->second.erase(std::remove_if(std::begin(iter->second), std::end(iter->second),
                                      [&](const std::pair<NodeId, Value>& pair) {
                                        return pair.first == source.sender_id;
                                      }), std::end(iter->second));
    iter->second.push_back(request);
  } else {
    kv_pairs_.insert(std::make_pair(request.first,
                                    std::make_pair(source.sender_id, request.second)));
  }
  std::vector values_vector;
  for (const auto& value : iter->second)
    values_vector.push_back(value->second);
  auto resolution(AccountType::Resolve(values_vector));
  if (resolution.result == AddResult::kSuccess) {
    kv_pairs_.erase(request.first);
    return Result(resolution.key, resolution.value, AddResult::kSuccess);
  } else if (resolution.result == AddResult::kFailure) {
    kv_pairs_.erase(request.first);
    return Result(request.first, std::unique_ptr<AccountType::Value>(), AddResult::kFailure);
  } else {
    return Result(request.first, std::vector<Value>(), AddResult::kWaiting);
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
