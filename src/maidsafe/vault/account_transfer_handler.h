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


namespace maidsafe {

namespace vault {

template <typename persona>
class AccountTransferHandler {
 public:
  using Key = typename persona::Key;
  using Value = typename persona::Value;

  enum class AddResult {
    kSuccess,
    kWaiting,
    kFailure
  };

  struct Result {
    Result(const Key& key_in,
           const boost::optional<Value> value_in,
           const AddResult& result_in):
        key(key_in), value(*value_in), result(result_in) {}
    Key key;
    boost::optional<Value> value;
    AddResult result;
  };


  AccountTransferHandler();

  Result Add(const Key &key, const Value &value, const NodeId& source_id);

  AccountTransferHandler(const AccountTransferHandler&) = delete;
  AccountTransferHandler& operator=(const AccountTransferHandler&) = delete;
  AccountTransferHandler(AccountTransferHandler&&) = delete;
  AccountTransferHandler& operator=(AccountTransferHandler&&) = delete;

 private:
  using SourceValuePair = std::pair<NodeId, Value>;

  std::map<Key, std::vector<SourceValuePair>> kv_pairs_;
  mutable std::mutex mutex_;
};

// ==================== Implementation =============================================================

template <typename Persona>
AccountTransferHandler<Persona>::AccountTransferHandler()
    : kv_pairs_(), mutex_() {}

template <typename Persona>
typename AccountTransferHandler<Persona>::Result
AccountTransferHandler<Persona>::Add(const typename Persona::Key& key,
    const typename Persona::Value& value, const NodeId& source_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter(kv_pairs_.find(key));
  if (iter != std::end(kv_pairs_)) {
    // replace entry from existing sender
    iter->second.erase(std::remove_if(std::begin(iter->second), std::end(iter->second),
                                      [&](const std::pair<NodeId, Value>& pair) {
                                        return pair.first == source_id;
                                      }), std::end(iter->second));
    iter->second.push_back(std::make_pair(source_id, value));
  } else {
    kv_pairs_.insert(
        std::make_pair(key, std::vector<SourceValuePair>{ std::make_pair(source_id, value) }));
  }
  std::vector<Value> values;
  for (const auto& pair : iter->second)
    values.push_back(pair.second);
  try {
    boost::optional<Value> resolved_value = Value::Resolve(values);
    kv_pairs_.erase(key);
    return Result(key, resolved_value, AddResult::kSuccess);
  } catch (const maidsafe::maidsafe_error& error) {
    if (error.code() != make_error_code(CommonErrors::unable_to_handle_request)) {
      // Unsuccessfull resolution
      kv_pairs_.erase(key);
      return Result(key, boost::optional<Value>(), AddResult::kFailure);
    } else if (error.code() != make_error_code(VaultErrors::too_few_entries_to_resolve)) {
      // resolution requires more entries
      return Result(key, boost::optional<Value>(), AddResult::kWaiting);
    } else {
      throw;
    }
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_HANDLER_H_
