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

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/vault.h"

namespace maidsafe {

namespace vault {

namespace test {


MaidManager<VaultFacade>::AccountName CreateAccount(MaidManager<VaultFacade>& maid_manager,
                                                    uint64_t space_offered) {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::PublicAnmaid public_anmaid(anmaid);
  passport::PublicMaid public_maid(maid);

  for (;;) {
    try {
      maid_manager.HandleCreateAccount(public_maid, public_anmaid, space_offered);
      EXPECT_TRUE(maid_manager.HasAccount(public_maid.Name()));
      break;
    }
    catch (const maidsafe_error& error) {
      EXPECT_EQ(make_error_code(VaultErrors::failed_to_handle_request), error.code());
      EXPECT_FALSE(maid_manager.HasAccount(public_maid.Name()));
    }
  }

  return public_maid.Name();
}

template <typename Data>
std::pair<routing::SourceAddress, Data> GetSourceAddressAndData(
    const MaidManager<VaultFacade>::AccountName account_name, uint32_t data_size) {
  Data data(NonEmptyString(RandomAlphaNumericString(data_size)));

  auto node_address(routing::NodeAddress(routing::Address(account_name.string())));
  auto group_address = boost::optional<routing::GroupAddress>();
  auto reply_to_address = boost::optional<routing::ReplyToAddress>();
  auto source_address(routing::SourceAddress(node_address, group_address, reply_to_address));

  return std::make_pair(source_address, data);
}


TEST(MaidManagerTest, BEH_CreateAccount) {
  MaidManager<VaultFacade> maid_manager;
 
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::PublicAnmaid public_anmaid(anmaid);
  passport::PublicMaid public_maid(maid);

  // FakeRouting has ~75% success rate for creating an account
  try {
    maid_manager.HandleCreateAccount(public_maid, public_anmaid);
    ASSERT_TRUE(maid_manager.HasAccount(public_maid.Name()));
    try {
      // try creating an account for the same client
      maid_manager.HandleCreateAccount(public_maid, public_anmaid);
    }
    catch (const maidsafe_error& error) {
      ASSERT_EQ(make_error_code(VaultErrors::account_already_exists), error.code());
    }
  }
  catch (const maidsafe_error& error) {
    ASSERT_EQ(make_error_code(VaultErrors::failed_to_handle_request), error.code());
    ASSERT_FALSE(maid_manager.HasAccount(public_maid.Name()));
  }
}

TEST(MaidManagerTest, BEH_Put) {
  using AccountName = MaidManager<VaultFacade>::AccountName;

  MaidManager<VaultFacade> maid_manager;
  AccountName account_name(CreateAccount(maid_manager, std::numeric_limits<uint64_t>().max()));

  // any reasonable number of puts should succeed 
  uint32_t puts((RandomUint32() % 1000) + 1);
  for (size_t i = 0; i != puts; ++i) {
    auto args(GetSourceAddressAndData<ImmutableData>(account_name, 10));
    auto result(maid_manager.HandlePut<ImmutableData>(args.first, args.second));

    ASSERT_TRUE(result.valid());
    ASSERT_EQ(1, result.value().size());
    ASSERT_EQ(args.second.Name().string(), result.value()[0].first.data.string());
  }
}

TEST(MaidManagerTest, BEH_PutToLimit) {
  using AccountName = MaidManager<VaultFacade>::AccountName;

  MaidManager<VaultFacade> maid_manager;
  AccountName account_name(CreateAccount(maid_manager, 120));

  for (size_t i = 0; i != 3; ++i) {
    auto args(GetSourceAddressAndData<ImmutableData>(account_name, 10));
    auto result(maid_manager.HandlePut<ImmutableData>(args.first, args.second));

    ASSERT_TRUE(result.valid());
    ASSERT_EQ(1, result.value().size());
    ASSERT_EQ(args.second.Name().string(), result.value()[0].first.data.string());
  }
}

TEST(MaidManagerTest, BEH_PutPastLimit) {
  using AccountName = MaidManager<VaultFacade>::AccountName;

  MaidManager<VaultFacade> maid_manager;
  AccountName account_name(CreateAccount(maid_manager, 120));

  for (size_t i = 0; i != 3; ++i) {
    auto args(GetSourceAddressAndData<ImmutableData>(account_name, 10));
    auto result(maid_manager.HandlePut<ImmutableData>(args.first, args.second));

    ASSERT_TRUE(result.valid());
    ASSERT_EQ(1, result.value().size());
    ASSERT_EQ(args.second.Name().string(), result.value()[0].first.data.string());
  }

  auto args(GetSourceAddressAndData<ImmutableData>(account_name, 10));
  auto result(maid_manager.HandlePut<ImmutableData>(args.first, args.second));

  ASSERT_TRUE(!result.valid());
  ASSERT_EQ(make_error_code(CommonErrors::cannot_exceed_limit), result.error().code());
}

TEST(MaidManagerTest, BEH_PutWithoutAccount) {
  using AccountName = MaidManager<VaultFacade>::AccountName;

  MaidManager<VaultFacade> maid_manager;
  AccountName account_name(Identity(RandomAlphaNumericBytes(64)));

  auto args(GetSourceAddressAndData<ImmutableData>(account_name, 10));
  auto result(maid_manager.HandlePut<ImmutableData>(args.first, args.second));

  ASSERT_TRUE(!result.valid());
  ASSERT_EQ(make_error_code(VaultErrors::no_such_account), result.error().code());
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
