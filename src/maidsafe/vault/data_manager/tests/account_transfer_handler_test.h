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
  using KeyValuePair = std::pair<typename DataManagerPersona::Key,
                                 typename DataManagerPersona::Value>;
  using KeyResultPair = std::pair<typename DataManagerPersona::Key,
                                  typename AccountTransferHandler<DataManagerPersona>::AddResult>;
  using Value = DataManagerPersona::Value;
  using Key = DataManagerPersona::Key;

  Value CreateValue(int32_t data_size) {
    return Value { PmidName {Identity {NodeId(NodeId::IdType::kRandomId) } },  data_size };
  }

  void Produce(unsigned int resolvable, unsigned int unresolvable,
               std::vector<KeyValuePair>& entries, std::vector<KeyResultPair>& result) {
    for (unsigned int index(); index < resolvable + unresolvable; ++index) {
    }
  }
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
