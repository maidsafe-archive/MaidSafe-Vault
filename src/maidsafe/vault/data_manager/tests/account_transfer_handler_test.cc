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


namespace maidsafe {

namespace vault {

namespace test {

TEST(AccountTransferHandlerTest, BEH_SucceedToResolve) {
  using TypedHandler = AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kDataManager>>;
  int32_t data_size(64);
  std::vector<NodeId> source_nodes;
  for (unsigned int index(0); index < routing::Parameters::group_size; ++index)
    source_nodes.push_back(NodeId(NodeId::IdType::kRandomId));

  TypedHandler account_transfer_handler;
  ImmutableData data { NonEmptyString { RandomAlphaNumericString(data_size) } };
  typename TypedHandler::Key key { data.name() };
  TypedHandler::Value value { PmidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
                              data_size };
  auto result(account_transfer_handler.Add(key, value, source_nodes.at(0)));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
  EXPECT_EQ(result.key, key);

  // replace existing entry with new one
  result = account_transfer_handler.Add(key, value, source_nodes.at(0));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
  EXPECT_EQ(result.key, key);

  // should handle account transfer
  result = account_transfer_handler.Add(key, value, source_nodes.at(1));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kSuccess);
  EXPECT_EQ(result.key, key);
  EXPECT_EQ(*result.value, value);
}

TEST(AccountTransferHandlerTest, BEH_FailToResolve) {
  using TypedHandler = AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kDataManager>>;
  int32_t data_size(64);
  std::vector<NodeId> source_nodes;
  for (unsigned int index(0); index < routing::Parameters::group_size; ++index)
    source_nodes.push_back(NodeId(NodeId::IdType::kRandomId));

  TypedHandler account_transfer_handler;
  ImmutableData data { NonEmptyString { RandomAlphaNumericString(data_size) } };
  typename TypedHandler::Key key { data.name() };
  TypedHandler::Value value { PmidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
                              data_size };
  auto result(account_transfer_handler.Add(key, value, source_nodes.at(0)));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
  EXPECT_EQ(result.key, key);

  // another holder sends different value
  result = account_transfer_handler.Add(
               key,
               TypedHandler::Value(PmidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
                                   data_size),
               source_nodes.at(1));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kWaiting);
  EXPECT_EQ(result.key, key);

  // another holder sends different value
  result = account_transfer_handler.Add(
               key,
               TypedHandler::Value(PmidName(Identity(NodeId(NodeId::IdType::kRandomId).string())),
                                   data_size),
               source_nodes.at(2));
  EXPECT_EQ(result.result, TypedHandler::AddResult::kFailure);
  EXPECT_EQ(result.key, key);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
