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

#include "maidsafe/vault/account_transfer_handler.h"

#include "maidsafe/common/test.h"
#include "maidsafe/common/data_types/immutable_data.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/data_manager/value.h"

#include "maidsafe/vault/tests/account_transfer_analyser.h"
#include "maidsafe/vault/tests/account_transfer_info_handler.h"
#include "maidsafe/vault/data_manager/tests/account_transfer_info_handler.h"

namespace maidsafe {

namespace vault {

namespace test {

template <typename Persona>
class AccountTransferHandlerTest : public testing::Test {
 public:
  using Key = typename Persona::Key;
  using Value = typename Persona::Value;
  using KeyValuePair = std::pair<typename Persona::Key, typename Persona::Value>;
  using KeyResultPair = std::pair<typename Persona::Key,
                                  typename AccountTransferHandler<Persona>::AddResult>;
  using AddResult = typename AccountTransferHandler<Persona>::AddResult;

  AccountTransferHandlerTest() : mutex_(), account_transfer_handler_(),
                                 account_transfer_analyser_() {}

 protected:
  std::mutex mutex_;
  AccountTransferHandler<Persona> account_transfer_handler_;
  AccountTransferAnalyser<Persona> account_transfer_analyser_;
};

TYPED_TEST_CASE_P(AccountTransferHandlerTest);

TYPED_TEST_P(AccountTransferHandlerTest, BEH_MultipleEntries) {
  using AddResult = typename AccountTransferHandlerTest<TypeParam>::AddResult;
  std::map<typename AccountTransferHandler<TypeParam>::Key,
           typename AccountTransferHandler<TypeParam>::Result> results_map;
  this->account_transfer_analyser_.CreateEntries(1000);
  this->account_transfer_analyser_.DefaultReplicate();
  this->account_transfer_analyser_.RandomReplicate(100);
  for (const auto& entry : this->account_transfer_analyser_.GetKeyValuePairs()) {
    auto add_result(this->account_transfer_handler_.Add(entry.first, entry.second,
                                                        NodeId(NodeId::IdType::kRandomId)));
    auto iter(results_map.find(entry.first));
    if (iter  == std::end(results_map)) {
      results_map.insert(std::make_pair(entry.first, add_result));
    } else if (results_map.at(entry.first).result != AddResult::kSuccess) {
      results_map.at(entry.first) = add_result;
    }
  }
  EXPECT_TRUE(this->account_transfer_analyser_.CheckResults(results_map));
}

TYPED_TEST_P(AccountTransferHandlerTest, BEH_Prune) {
  using AddResult = typename AccountTransferHandlerTest<TypeParam>::AddResult;
  std::map<Key, typename AccountTransferHandler<TypeParam>::Result> results_map;
  this->account_transfer_analyser_.CreateEntries(
      detail::Parameters::account_transfer_cleanup_factor - 1);
  auto pairs(this->account_transfer_analyser_.GetKeyValuePairs());
  Sleep(detail::Parameters::account_transfer_life);
  this->account_transfer_analyser_.CreateEntries(
      detail::Parameters::account_transfer_cleanup_factor + 1);
  for (const auto& entry : pairs) {
    for (unsigned int index(0); index < this->account_transfer_analyser_.kAcceptSize() - 1;
         ++index) {
      auto add_result(this->account_transfer_handler_.Add(entry.first, entry.second,
                                                          NodeId(NodeId::IdType::kRandomId)));
      EXPECT_EQ(add_result.result, AddResult::kWaiting);
    }
  }
}

TYPED_TEST_P(AccountTransferHandlerTest, BEH_ParallelMultipleEntries) {
  std::map<typename AccountTransferHandler<TypeParam>::Key,
           typename AccountTransferHandler<TypeParam>::Result> results_map;
  using TypedHandlerTest = AccountTransferHandlerTest<TypeParam>;
  const unsigned int kParallelismFactor(10);
  unsigned int index(0);
  this->account_transfer_analyser_.CreateEntries(900);
  this->account_transfer_analyser_.DefaultReplicate();
  std::vector<std::vector<typename TypedHandlerTest::KeyValuePair>>
      parallel_inputs(kParallelismFactor, std::vector<typename TypedHandlerTest::KeyValuePair>());

  parallel_inputs.reserve(kParallelismFactor);
  for (const auto& entry : this->account_transfer_analyser_.GetKeyValuePairs())
    parallel_inputs.at(index++ % kParallelismFactor).push_back(entry);

  std::vector<std::future<void>> futures;
  for (unsigned int index(0); index < kParallelismFactor; ++index) {
    auto& vector_ref(parallel_inputs.at(index));
    futures.emplace_back(std::async(std::launch::async,
                         [vector_ref, index, &results_map, this]() {
                           for (const auto& entry : vector_ref) {
                             auto add_result(
                                 this->account_transfer_handler_.Add(
                                     entry.first, entry.second, NodeId(NodeId::IdType::kRandomId)));
                             {
                               std::lock_guard<std::mutex> lock(this->mutex_);
                               auto iter(results_map.find(entry.first));
                               if (iter  == std::end(results_map)) {
                                 results_map.insert(std::make_pair(entry.first, add_result));
                               } else if (add_result.result !=
                                     TypedHandlerTest::AddResult::kWaiting) {
                                 results_map.at(entry.first) = add_result;
                               }
                             }
                           }
                         }));
  }
  for (unsigned int index(0); index < kParallelismFactor; ++index)
    futures.at(index).get();
  EXPECT_TRUE(this->account_transfer_analyser_.CheckResults(results_map));
}

REGISTER_TYPED_TEST_CASE_P(AccountTransferHandlerTest, BEH_MultipleEntries, BEH_Prune,
                           BEH_ParallelMultipleEntries);
typedef testing::Types<MaidManager, DataManager, PmidManager> PersonaTypes;
INSTANTIATE_TYPED_TEST_CASE_P(AccountTransfer, AccountTransferHandlerTest, PersonaTypes);

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
