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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_ACCCOUNT_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_ACCCOUNT_H_

#include "maidsafe/vault/account_transfer.h"
#include "maidsafe/vault/data_manager/data_manager.h"

namespace maidsafe {

namespace vault {

class DataManagerAccount {
  typedef DataManager::Key Key;
  typedef DataManager::Value Value;

  explicit DataManagerAccount(Db<Key, Value>& db) : db_(db) {}
  static AccountTransferHandler<DataManagerAccount>::Result
  Resolve(const Key& key, const std::vector<Value>& values);

 private:
  static void HandleAccountTransfer(const Key& key, const Value& value);
  Db<Key, Value>& db_;
};

AccountTransferHandler<DataManagerAccount>::Result
DataManagerAccount::Resolve(const Key& key, const std::vector<Value>& values) {
  std::map<DataManagerAccountType::Value, size_t> stats;
  for (const auto& value : values)
    stats.at(value)++;

  auto max_iter(stats.begin());
  for (auto iter(std::begin(stats)); iter != std::end(stats); ++iter)
    max_iter = (iter->second > max) ? iter : max_iter;

  if (max_iter->second == (Parameters::group_size + 1) / 2) {
    HandleAccountTransfer(key, max_iter->first);
    return AccountTransferHandler<DataManagerAccount>::Result(key, max_iter->first,
                                                              AddResult::kSuccess);
  }

  if (max_iter->second == Parameters::group_size - 1)
    return AccountTransferHandler<DataManagerAccount>::Result(key, max_iter->first,
                                                              AddResult::kFailure);

  return AccountTransferHandler<DataManagerAccount>::Result(key, max_iter->first,
                                                            AddResult::kWaiting);
}

void DataManagerAccount::HandleAccountTransfer(const Key& key, const Value& value) {
  kv_pairs.push_back(std::make_pair(key, std::move(value)));
  db_.HandleTransfer(kv_pairs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_ACCCOUNT_H_
