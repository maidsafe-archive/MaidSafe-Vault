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
#include "maidsafe/vault/data_manager/data_manager.h"

#include "maidsafe/vault/tests/expected_account_transfer_producer.h"
#include "maidsafe/vault/data_manager/tests/account_transfer_handler_test.h"

namespace maidsafe {

namespace vault {

namespace test {

template <typename Persona>
class AccountTransferHandlerTest : public testing::Test,
                                   public ExpectedAccountTransferProducer<Persona> {
 public:
  using Key = typename Persona::Key;
  using Value = typename Persona::Value;
  using KeyValuePair = std::pair<typename Persona::Key, typename Persona::Value>;
  using KeyResultPair = std::pair<typename Persona::Key,
                                  typename AccountTransferHandler<Persona>::AddResult>;

  AccountTransferHandlerTest() : account_transfer_handler_() {}

 protected:
   AccountTransferHandler<Persona> account_transfer_handler_;
};

TYPED_TEST_CASE_P(AccountTransferHandlerTest);

TYPED_TEST_P(AccountTransferHandlerTest, BEH_InputMultipleEntries) {
  std::map<Key, typename AccountTransferHandler<TypeParam>::AddResult> results_map;
  auto results(ExpectedAccountTransferProducer<TypeParam>::ProduceResults(1000));
  for (const auto& entry : ExpectedAccountTransferProducer<TypeParam>::kv_pairs_) {
    auto add_result(this->account_transfer_handler_.Add(entry.first, entry.second,
                                                        NodeId(NodeId::IdType::kRandomId)));
    auto iter(results_map.find(entry.first));
    if (iter  == std::end(results_map)) {
      results_map.insert(std::make_pair(entry.first, add_result.result));
    } else {
      results_map[entry.first] = add_result.result;
    }
  }
  EXPECT_EQ(results.size(), results_map.size());
  for (const auto& entry : results)
    EXPECT_EQ(entry.second, results_map.at(entry.first));
}

REGISTER_TYPED_TEST_CASE_P(AccountTransferHandlerTest, BEH_InputMultipleEntries);
typedef testing::Types<nfs::PersonaTypes<nfs::Persona::kDataManager>> PersonaTypes;
INSTANTIATE_TYPED_TEST_CASE_P(AccountTransfer, AccountTransferHandlerTest, PersonaTypes);

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
