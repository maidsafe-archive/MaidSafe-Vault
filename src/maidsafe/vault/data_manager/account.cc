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

#include "maidsafe/vault/data_manager/account.h"

namespace maidsafe {

namespace vault {

typename DataManagerAccount::AccountTransferType::Result
DataManagerAccount::Resolve(const Key& key, const std::vector<std::pair<NodeId, Value>>& values) {
  std::vector<std::pair<Value, unsigned int>> stats;
  auto max_iter(std::begin(stats));
  for (const auto& value : values) {
    auto iter(std::find_if(std::begin(stats), std::end(stats),
                           [&](const std::pair<Value, unsigned int>& pair_value) {
                             return value.second == pair_value.first;
                           }));
    if (iter == std::end(stats))
      stats.emplace_back(std::make_pair(value.second, 0));
    else
      iter->second++;
    max_iter = (iter->second > max_iter->second) ? iter : max_iter;
  }

  if (max_iter->second == (routing::Parameters::group_size + 1) / 2) {
    return AccountTransferType::Result(key, boost::optional<Value>(max_iter->first),
                                       AccountTransferType::AddResult::kSuccess);
  }

  if (max_iter->second == routing::Parameters::group_size - 1)
    return AccountTransferType::Result(key, boost::optional<Value>(max_iter->first),
                                       AccountTransferType::AddResult::kFailure);

  return AccountTransferType::Result(key, boost::optional<Value>(max_iter->first),
                                     AccountTransferType::AddResult::kWaiting);
}

}  // namespace vault

}  // namespace maidsafe


