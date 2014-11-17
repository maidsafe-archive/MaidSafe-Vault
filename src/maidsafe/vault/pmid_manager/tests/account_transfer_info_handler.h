/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/tests/account_transfer_analyser.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/utils.h"

template <typename Persona> class AccountTransferHandler;

namespace maidsafe {

namespace vault {

namespace test {

using PmidManagerPersona = typename nfs::PersonaTypes<nfs::Persona::kPmidManager>;

template <>
class AccountTransferInfoHandler<PmidManagerPersona> {
 public:
  using Value = PmidManagerPersona::Value;
  using Key = PmidManagerPersona::Key;

  using KeyValuePair = std::pair<Key, Value>;
  using Result = AccountTransferHandler<PmidManagerPersona>::Result;
  using AddResult = AccountTransferHandler<PmidManagerPersona>::AddResult;

  AccountTransferInfoHandler()
      : kAcceptSize_((routing::Parameters::group_size + 1) / 2),
        kResolutionSize_(kAcceptSize_) {}

  KeyValuePair CreatePair() {
    return std::make_pair(Key(Identity { NodeId(NodeId::IdType::kRandomId).string() }),
                          CreateValue());
  }

  std::pair<AddResult, boost::optional<Value>> Resolve(const std::vector<KeyValuePair>& pairs) {
    std::pair<AddResult, boost::optional<Value>>
        wait_resolution(std::make_pair(AddResult::kWaiting, boost::optional<Value>()));
    if (pairs.size() < kResolutionSize_)
      return wait_resolution;

    auto limit_iter(std::begin(pairs));
    std::advance(limit_iter, kResolutionSize_);
    std::vector<KeyValuePair> resolution_pairs { std::begin(pairs), limit_iter };
    return ApplyMedian(resolution_pairs);
  }

  std::pair<AddResult, boost::optional<Value>> ApplyMedian(
      const std::vector<KeyValuePair>& pairs) {
    std::vector<int64_t> stored_total_size, lost_total_size, offered_space;
    for (const auto& value : pairs) {
      stored_total_size.emplace_back(value.second.stored_total_size);
      lost_total_size.emplace_back(value.second.lost_total_size);
      offered_space.emplace_back(value.second.offered_space);
    }

    PmidManagerValue value;
    value.stored_total_size = Median(stored_total_size);
    value.lost_total_size = Median(lost_total_size);
    value.offered_space = Median(offered_space);

    return std::make_pair(AddResult::kSuccess, boost::optional<Value>(value));
  }

  std::vector<Result> ProduceResults(const std::vector<KeyValuePair>& kv_pairs) {
    std::map<Key, Result> results;
    std::vector<KeyValuePair> copy(kv_pairs);
    std::vector<KeyValuePair> same_key_pairs;
    for (const auto& kv_pair : kv_pairs) {
      if (results.find(kv_pair.first) == std::end(results)) {
        for (const auto& entry : copy)
          if (entry.first == kv_pair.first)
            same_key_pairs.push_back(entry);
        auto verdict(Resolve(same_key_pairs));
        if (verdict.first == AddResult::kSuccess)
          results.insert(
              std::make_pair(kv_pair.first,
                             Result(kv_pair.first, boost::optional<Value>(verdict.second),
                                    AddResult::kSuccess)));
        else
          results.insert(std::make_pair(kv_pair.first,
                                        Result(kv_pair.first, boost::optional<Value>(),
                                               AddResult::kWaiting)));
        same_key_pairs.clear();
      }
    }
    std::vector<Result> results_vec;
    results_vec.reserve(results.size());
    for (const auto& result : results)
      results_vec.push_back(result.second);
    return results_vec;
  }

  Value CreateValue() {
    return Value(RandomUint32() % 1000, RandomUint32() % 1000, RandomUint32() % 1000);
  }

  unsigned int kAcceptSize() const {
    return kAcceptSize_;
  }

  unsigned int kResolutionSize() const {
    return kResolutionSize_;
  }

 private:
  const unsigned int kAcceptSize_, kResolutionSize_;
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_
