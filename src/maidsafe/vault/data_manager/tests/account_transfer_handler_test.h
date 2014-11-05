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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_TEST_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_TEST_ACCOUNT_TRANSFER_H_

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/tests/expected_account_transfer_producer.h"
#include "maidsafe/vault/data_manager/data_manager.h"

template <typename Persona> class AccountTransferHandler;

namespace maidsafe {

namespace vault {

namespace test {

using DataManagerPersona = typename nfs::PersonaTypes<nfs::Persona::kDataManager>;

template <>
class ExpectedAccountTransferProducer<DataManagerPersona> {
 public:
  using Value = DataManagerPersona::Value;
  using Key = DataManagerPersona::Key;

  using KeyValuePair = std::pair<Key, Value>;
  using AddResult = AccountTransferHandler<DataManagerPersona>::AddResult;

  ExpectedAccountTransferProducer()
      : kv_pairs_(), kDataSize_(64), kAcceptSize_((routing::Parameters::group_size + 1) / 2),
        kFailureSize_(routing::Parameters::group_size - 1) {}

  std::map<Key, AddResult> ProduceResults(unsigned int total) {
    CreateEntries(total);
    RandomlyDistribute();
    std::map<Key, AddResult> results;
    auto copy(kv_pairs_);
    for (const auto& kv_pair : kv_pairs_) {
      if (results.find(kv_pair.first) == std::end(results)) {
        results.insert(std::make_pair(kv_pair.first, AddResult::kWaiting));
        if (std::count(std::begin(copy), std::end(copy), kv_pair) >= kAcceptSize_)
          results[kv_pair.first] = AddResult::kSuccess;
        else if (std::count_if(std::begin(copy), std::end(copy),
                               [&kv_pair](const KeyValuePair& pair) {
                                 return pair.first == kv_pair.first;
                               }) >= kFailureSize_)
          results[kv_pair.first] = AddResult::kFailure;
      }
    }
    return results;
  }

 protected:
  std::vector<KeyValuePair> kv_pairs_;

 private:
  void CreateEntries(unsigned int total) {
    for (unsigned int index(0); index < total; ++index) {
      kv_pairs_.push_back(
          std::make_pair(Key(Identity { NodeId(NodeId::IdType::kRandomId).string() },
                         ImmutableData::Tag::kValue), CreateValue(kDataSize_)));
      LOG(kVerbose) << kv_pairs_.size() << " added "
                    << HexSubstr(kv_pairs_.back().first.name.string());
    }
  }

  void RandomlyDistribute() {
    assert(!kv_pairs_.empty());
    auto original_size(kv_pairs_.size());
    std::vector<KeyValuePair>::iterator resolvable_end_iter(std::begin(kv_pairs_)),
        unresolvable_iter;
    std::advance(resolvable_end_iter, original_size / 2);
    CreateResolvables(resolvable_end_iter);
    resolvable_end_iter = unresolvable_iter = std::begin(kv_pairs_);
    std::advance(resolvable_end_iter, original_size / 2 + 1);
    std::advance(unresolvable_iter, original_size * 3 / 4);
    CreateUnResolvable(resolvable_end_iter, unresolvable_iter);
  }

  Value CreateValue(int32_t data_size) {
    return Value { PmidName { Identity { NodeId(NodeId::IdType::kRandomId).string() } },
                   data_size };
  }

  void CreateResolvables(std::vector<KeyValuePair>::iterator& resolvable_end_iter) {
    std::vector<KeyValuePair> new_pairs;
    for (auto iter(std::begin(kv_pairs_)); iter != resolvable_end_iter; ++iter)
      for (unsigned int index(0); index < kAcceptSize_ - 1; ++index)
        new_pairs.push_back(*iter);
    std::copy(std::begin(new_pairs), std::end(new_pairs), std::back_inserter(kv_pairs_));
  }

  void CreateUnResolvable(std::vector<KeyValuePair>::iterator& unresolvable_start_iter,
                          std::vector<KeyValuePair>::iterator& unresolvable_end_iter) {
    std::vector<KeyValuePair> new_pairs;
    for (; unresolvable_start_iter != unresolvable_end_iter; ++unresolvable_start_iter)
      for (unsigned int index(0); index < kFailureSize_ - 1; ++index) {
        new_pairs.push_back(
            std::make_pair(unresolvable_start_iter->first, CreateValue(kDataSize_)));
      }
    std::copy(std::begin(new_pairs), std::end(new_pairs), std::back_inserter(kv_pairs_));
  }

  const int32_t kDataSize_;
  const unsigned int kAcceptSize_, kFailureSize_;
};

//TYPED_TEST_P(AccountTransferHandlerTest, BEH_SuccessfulResolve) {
//  using TypedHandler = AccountTransferHandler<TypeParam>;
//  auto value(this->CreateValue());
//  auto key(this->GetKey());
//  auto result(this->account_transfer_handler_.Add(key, value, this->source_ids_.at(0)));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
//  EXPECT_EQ(result.key, key);

//  // replace existing entry with new one
//  result = this->account_transfer_handler_.Add(key, value, this->source_ids_.at(0));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
//  EXPECT_EQ(result.key, key);

//  // should handle account transfer
//  result = this->account_transfer_handler_.Add(key, value, this->source_ids_.at(1));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kSuccess);
//  EXPECT_EQ(result.key, key);
//  EXPECT_EQ(*result.value, value);
//}

//TYPED_TEST_P(AccountTransferHandlerTest, BEH_FailToResolve) {
//  using TypedHandler = AccountTransferHandler<TypeParam>;
//  auto key(this->GetKey());
//  auto value(this->CreateValue());
//  auto result(this->account_transfer_handler_.Add(key, value, this->source_ids_.at(0)));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
//  EXPECT_EQ(result.key, key);

//  // another holder sends different value
//  result = this->account_transfer_handler_.Add(key, this->CreateValue(), this->source_ids_.at(1));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
//  EXPECT_EQ(result.key, key);

//  // another holder sends different value
//  result = this->account_transfer_handler_.Add(key, this->CreateValue(), this->source_ids_.at(2));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kFailure);
//  EXPECT_EQ(result.key, key);

//  // should not resolve as the handler should be empty
//  result = this->account_transfer_handler_.Add(key, value, this->source_ids_.at(0));
//  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
//  EXPECT_EQ(result.key, key);
//}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_TEST_ACCOUNT_TRANSFER_H_
