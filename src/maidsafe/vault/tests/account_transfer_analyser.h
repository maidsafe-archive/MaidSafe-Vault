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

#ifndef MAIDSAFE_VAULT_TESTS_ACCOUNT_TRANSFER_ANALYSER_H_
#define MAIDSAFE_VAULT_TESTS_ACCOUNT_TRANSFER_ANALYSER_H_

#include <algorithm>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/optional.hpp"

#include "maidsafe/common/node_id.h"

#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"


namespace maidsafe {

namespace vault {

namespace test {

template <typename Persona> class  AccountTransferInfoHandler;

template <typename Persona>
class AccountTransferAnalyser : public AccountTransferInfoHandler<Persona> {
 public:
  using Value = typename Persona::Value;
  using Key = typename Persona::Key;

  using KeyValuePair = std::pair<Key, Value>;
  using Result = typename AccountTransferHandler<Persona>::Result;
  using AddResult = typename AccountTransferHandler<Persona>::AddResult;

  AccountTransferAnalyser();
  bool CheckResults(
      const std::map<Key, typename AccountTransferHandler<Persona>::Result>& results_in);
  void CreateEntries(unsigned int total);
  std::vector<KeyValuePair> GetKeyValuePairs() const;
  void DefaultReplicate();
  void RandomReplicate(unsigned int replicates);
  unsigned int kAcceptSize();
  unsigned int kResolutionSize();

 private:
  void ReplicateWithSameValue(typename std::vector<KeyValuePair>::iterator& start_iter,
                              typename std::vector<KeyValuePair>::iterator& end_iter,
                              unsigned int quantity = 1);
  void ReplicateWithDifferentValue(typename std::vector<KeyValuePair>::iterator& start_iter,
                                   typename std::vector<KeyValuePair>::iterator& end_iter,
                                   unsigned int quantity = 1);
  std::string Print(const Key& key);

  std::vector<KeyValuePair> kv_pairs_;
};

template <typename Persona>
AccountTransferAnalyser<Persona>::AccountTransferAnalyser()
    : kv_pairs_() {}

template <typename Persona>
bool AccountTransferAnalyser<Persona>::CheckResults(
    const std::map<Key, typename AccountTransferHandler<Persona>::Result>& results_in) {
  auto expected_results(AccountTransferInfoHandler<Persona>::ProduceResults(kv_pairs_));
  if (results_in.size() != expected_results.size())
    return false;
  for (const auto& expected_result : expected_results)
    if (!expected_result.Equals(results_in.at(expected_result.key))) {
      LOG(kError) << static_cast<int>(expected_result.result) << ", "
                  << static_cast<int>(results_in.at(expected_result.key).result);
      LOG(kError) << expected_result.value->Print() << ", "
                  << results_in.at(expected_result.key).value->Print();
      LOG(kError)  << "Print\n" << Print(expected_result.key);
      return false;
    }
  return true;
}

template <typename Persona>
unsigned int AccountTransferAnalyser<Persona>::kAcceptSize() {
  return AccountTransferInfoHandler<Persona>::kAcceptSize();
}

template <typename Persona>
unsigned int AccountTransferAnalyser<Persona>::kResolutionSize() {
  return AccountTransferInfoHandler<Persona>::kResolutionSize();
}

template <typename Persona>
std::string AccountTransferAnalyser<Persona>::Print(const Key& key) {
  std::stringstream stream;
  for (const auto& kv_pairs : kv_pairs_)
    if (kv_pairs.first == key)
      stream << kv_pairs.second.Print();
  return stream.str();
}

template <typename Persona>
void AccountTransferAnalyser<Persona>::CreateEntries(unsigned int total) {
  for (unsigned int index(0); index < total; ++index)
    kv_pairs_.push_back(AccountTransferInfoHandler<Persona>::CreatePair());
}

template <typename Persona>
std::vector<typename AccountTransferAnalyser<Persona>::KeyValuePair>
AccountTransferAnalyser<Persona>::GetKeyValuePairs() const {
  return kv_pairs_;
}

template <typename Persona>
void AccountTransferAnalyser<Persona>::DefaultReplicate() {
  assert(!kv_pairs_.empty());
  auto original_size(kv_pairs_.size());
  typename std::vector<KeyValuePair>::iterator start_iter(std::begin(kv_pairs_)),
      end_iter(std::begin(kv_pairs_));
  std::advance(end_iter, original_size / 2);
  ReplicateWithSameValue(start_iter, end_iter,
                         AccountTransferInfoHandler<Persona>::kAcceptSize() - 1);
  end_iter = start_iter = std::begin(kv_pairs_);
  std::advance(start_iter, original_size / 2 + 1);
  std::advance(end_iter, original_size * 3 / 4);
  ReplicateWithDifferentValue(start_iter, end_iter,
                              AccountTransferInfoHandler<Persona>::kResolutionSize() - 1);
  std::srand (unsigned(std::time(0)));
  std::random_shuffle(std::begin(kv_pairs_), std::end(kv_pairs_));
}

template <typename Persona>
void AccountTransferAnalyser<Persona>::RandomReplicate(unsigned int replicates) {
  bool same_value(true);
  typename std::vector<KeyValuePair>::iterator start_iter, end_iter;
  for (; replicates > 1; --replicates) {
    auto random_index(RandomUint32() % (kv_pairs_.size() - 1) + 1);
    start_iter = std::begin(kv_pairs_);
    std::advance(start_iter, random_index);
    end_iter = start_iter;
    std::advance(end_iter, 1);
    if (same_value)
      ReplicateWithSameValue(start_iter, end_iter);
    else
      ReplicateWithDifferentValue(start_iter, end_iter);
    same_value = !same_value;
  }
  std::srand (unsigned(std::time(0)));
  std::random_shuffle(std::begin(kv_pairs_), std::end(kv_pairs_));
}

template <typename Persona>
void AccountTransferAnalyser<Persona>::ReplicateWithSameValue(
    typename std::vector<KeyValuePair>::iterator& start_iter,
    typename std::vector<KeyValuePair>::iterator& end_iter,
    unsigned int quantity) {
  std::vector<KeyValuePair> new_pairs;
  for (; start_iter != end_iter; ++start_iter)
    for (unsigned int index(0); index < quantity; ++index)
      new_pairs.push_back(*start_iter);
  std::copy(std::begin(new_pairs), std::end(new_pairs), std::back_inserter(kv_pairs_));
}

template <typename Persona>
void AccountTransferAnalyser<Persona>::ReplicateWithDifferentValue(
    typename std::vector<KeyValuePair>::iterator& start_iter,
    typename std::vector<KeyValuePair>::iterator& end_iter,
    unsigned int quantity) {
  std::vector<KeyValuePair> new_pairs;
  for (; start_iter != end_iter; ++start_iter)
    for (unsigned int index(0); index < quantity; ++index) {
      new_pairs.push_back(
          std::make_pair(start_iter->first,
                         AccountTransferInfoHandler<Persona>::CreateValue()));
    }
  std::copy(std::begin(new_pairs), std::end(new_pairs), std::back_inserter(kv_pairs_));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_ACCOUNT_TRANSFER_ANALYSER_H_
