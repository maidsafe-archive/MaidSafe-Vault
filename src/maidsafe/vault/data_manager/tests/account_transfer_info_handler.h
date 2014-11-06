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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/tests/account_transfer_analyser.h"
#include "maidsafe/vault/data_manager/data_manager.h"

template <typename Persona> class AccountTransferHandler;

namespace maidsafe {

namespace vault {

namespace test {

using DataManagerPersona = typename nfs::PersonaTypes<nfs::Persona::kDataManager>;

template <>
class AccountTransferInfoHandler<DataManagerPersona> {
 public:
  using Value = DataManagerPersona::Value;
  using Key = DataManagerPersona::Key;

  using KeyValuePair = std::pair<Key, Value>;
  using Result = AccountTransferHandler<DataManagerPersona>::Result;
  using AddResult = AccountTransferHandler<DataManagerPersona>::AddResult;

  AccountTransferInfoHandler()
      : kDataSize_(64), kAcceptSize_((routing::Parameters::group_size + 1) / 2),
        kResolutionSize_(routing::Parameters::group_size - 1) {}

  KeyValuePair CreatePair() {
    return std::make_pair(Key(Identity { NodeId(NodeId::IdType::kRandomId).string() },
                              ImmutableData::Tag::kValue),
                          CreateValue());
  }

  std::vector<Result> ProduceResults(const std::vector<KeyValuePair>& kv_pairs) {
    std::map<Key, Result> results;
    auto copy(kv_pairs);
    for (const auto& kv_pair : kv_pairs) {
      if (results.find(kv_pair.first) == std::end(results)) {
        if (std::count(std::begin(copy), std::end(copy), kv_pair) >= kAcceptSize_)
          results.insert(
              std::make_pair(kv_pair.first,
                             Result(kv_pair.first, boost::optional<Value>(kv_pair.second),
                                    AddResult::kSuccess)));
        else if (std::count_if(std::begin(copy), std::end(copy),
                               [&kv_pair](const KeyValuePair& pair) {
                                 return pair.first == kv_pair.first;
                               }) >= kResolutionSize_)
          results.insert(std::make_pair(kv_pair.first,
                                        Result(kv_pair.first, boost::optional<Value>(),
                                               AddResult::kFailure)));
        else
          results.insert(std::make_pair(kv_pair.first,
                                        Result(kv_pair.first, boost::optional<Value>(),
                                               AddResult::kWaiting)));
      }
    }
    std::vector<Result> results_vec;
    results_vec.reserve(results.size());
    for (const auto& result : results)
      results_vec.push_back(result.second);
    return results_vec;
  }

  Value CreateValue() {
    return Value { PmidName { Identity { NodeId(NodeId::IdType::kRandomId).string() } },
                   kDataSize_ };
  }

  unsigned int AcceptSize() const {
    return kAcceptSize_;
  }

  unsigned int ResolutionSize() const {
    return kResolutionSize_;
  }

 private:
  const int32_t kDataSize_;
  const unsigned int kAcceptSize_, kResolutionSize_;
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_TESTS_ACCOUNT_TRANSFER_INFO_HANDLER_H_
