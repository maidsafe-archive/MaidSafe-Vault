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
#include <mutex>
#include <utility>
#include <vector>

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/global_fun.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/identity.hpp"
#include "boost/optional.hpp"

#include "maidsafe/common/clock.h"
#include "maidsafe/common/node_id.h"

#include "maidsafe/vault/parameters.h"

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
        key(key_in), value(value_in), result(result_in) {}
    bool Equals(const Result& other) const {
      return (result == other.result) && (key == other.key) && (value == other.value);
    }
    Key key;
    boost::optional<Value> value;
    AddResult result;
  };

  AccountTransferHandler();

  Result Add(const Key& key, const Value& value, const NodeId& source_id);

  AccountTransferHandler(const AccountTransferHandler&) = delete;
  AccountTransferHandler& operator=(const AccountTransferHandler&) = delete;
  AccountTransferHandler(AccountTransferHandler&&) = delete;
  AccountTransferHandler& operator=(AccountTransferHandler&&) = delete;

 private:
  void Prune(std::unique_lock<std::mutex>& lock);

  using SourceValuePair = std::pair<NodeId, Value>;

  struct Entry {
    Entry(const Key& key_in, const Value& value, const NodeId& source)
        : key(key_in), values(1, std::make_pair(source, value)),
          update_time(common::Clock::now()) {}
    Key key;
    std::vector<SourceValuePair> values;

    common::Clock::time_point update_time;
  };

  struct AccountKey {};
  struct AccountUpdateTime {};

  typedef boost::multi_index_container<
      Entry,
      boost::multi_index::indexed_by<
          boost::multi_index::ordered_unique<
              boost::multi_index::tag<AccountKey>,
              BOOST_MULTI_INDEX_MEMBER(Entry, Key, key)>,
          boost::multi_index::ordered_non_unique<
              boost::multi_index::tag<AccountUpdateTime>,
              BOOST_MULTI_INDEX_MEMBER(Entry, common::Clock::time_point, update_time)>>
  > AccountsContainer;

  AccountsContainer container_;
  mutable std::mutex mutex_;
};

// ==================== Implementation =============================================================

template <typename Persona>
AccountTransferHandler<Persona>::AccountTransferHandler()
    : container_(), mutex_() {}

template <typename Persona>
typename AccountTransferHandler<Persona>::Result
AccountTransferHandler<Persona>::Add(const typename Persona::Key& key,
    const typename Persona::Value& value, const NodeId& source_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  using  accounts_by_key = typename boost::multi_index::index<AccountsContainer, AccountKey>::type;
  accounts_by_key& key_index = boost::multi_index::get<AccountKey>(container_);
  auto iter(key_index.find(key));
  if (iter != std::end(key_index)) {
    key_index.modify(
        iter,
        [&](Entry& entry) {
          // replace entry from existing sender
          entry.values.erase(std::remove_if(std::begin(entry.values), std::end(entry.values),
                             [&](const std::pair<NodeId, Value>& pair) {
                               return pair.first == source_id;
                             }), std::end(entry.values));
          entry.values.push_back(std::make_pair(source_id, value));
          entry.update_time = common::Clock::now();
        });
  } else {
    container_.insert(Entry(key, value, source_id));
    iter = key_index.find(key);
  }
  std::vector<Value> values;
  for (const auto& pair : iter->values)
    values.push_back(pair.second);
  if (container_.size() % detail::Parameters::account_transfer_cleanup_factor == 0)
    Prune(lock);
  try {
    boost::optional<Value> resolved_value(Value::Resolve(values));
    container_.erase(key);
    return Result(key, resolved_value, AddResult::kSuccess);
  } catch (const maidsafe::maidsafe_error& error) {
    if (error.code() == make_error_code(VaultErrors::failed_to_handle_request)) {
      // Unsuccessfull resolution
      container_.erase(key);
      return Result(key, boost::optional<Value>(), AddResult::kFailure);
    } else if (error.code() == make_error_code(VaultErrors::too_few_entries_to_resolve)) {
      // resolution requires more entries
      return Result(key, boost::optional<Value>(), AddResult::kWaiting);
    } else {
      throw;
    }
  }
}

template <typename Persona>
void AccountTransferHandler<Persona>::Prune(std::unique_lock<std::mutex>& lock) {
  assert(lock.owns_lock());
  static_cast<void>(lock);
  using accounts_by_update_time = typename boost::multi_index::index<AccountsContainer,
                                                                     AccountUpdateTime>::type;
  accounts_by_update_time& update_time_index =
      boost::multi_index::get<AccountUpdateTime>(container_);

  auto dummy(*std::begin(update_time_index));
  auto upper(std::upper_bound(
      std::begin(update_time_index), std::end(update_time_index), dummy,
      [this](const Entry& lhs, const Entry& rhs) {
        return (lhs.update_time - rhs.update_time <
                    detail::Parameters::account_transfer_life);
      }));
  if (upper != std::end(update_time_index))
    update_time_index.erase(std::begin(update_time_index), upper);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_HANDLER_H_
