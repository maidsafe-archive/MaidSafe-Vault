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

#include "boost/filesystem.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"

#include "maidsafe/vault/vault.h"

namespace maidsafe {

namespace vault {

namespace test {

class DataManagerTest : public testing::Test {
 public:
  DataManagerTest() = default;

 protected:
  DataManager<VaultFacade> data_manager_ {
                               *maidsafe::test::CreateTestPath("MaidSafe_Vault_DataManager") };
};

TEST_F(DataManagerTest, BEH_HandlePutGet) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  routing::SourceAddress from(routing::NodeAddress(NodeId(RandomString(identity_size))),
                              boost::none, boost::none);
  auto put_result(data_manager_.HandlePut(from, data));
  EXPECT_TRUE(put_result.valid());
  auto& put_pmid_holder(put_result.value());
  EXPECT_EQ(put_result.value().size(), 4);
  auto get_result(data_manager_.HandleGet<ImmutableData>(from, Identity(data.name()->string())));
  EXPECT_TRUE(get_result.valid());
  auto& pmid_holders(boost::get<std::vector<routing::DestinationAddress>>(get_result.value()));
  EXPECT_EQ(pmid_holders.size(), put_result.value().size());
  for (const auto& get_pmid_holder : pmid_holders)
    EXPECT_TRUE(std::any_of(put_pmid_holder.begin(), put_pmid_holder.end(),
                            [&](const routing::DestinationAddress& put_address) {
                              return put_address.first.data == get_pmid_holder.first.data;
                            }));
}

TEST_F(DataManagerTest, BEH_HandlePostResponseNoAccount) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  routing::DestinationAddress destination(routing::Destination(NodeId(RandomString(identity_size))),
                                          boost::none);
  auto result(data_manager_.HandlePutResponse<ImmutableData>(
                  data.name(), destination, maidsafe_error(VaultErrors::data_already_exists)));
  EXPECT_FALSE(result.valid());
  EXPECT_EQ(result.error().code(), make_error_code(VaultErrors::no_such_account));
}

TEST_F(DataManagerTest, BEH_HandlePostResponse) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  routing::SourceAddress from(routing::NodeAddress(NodeId(RandomString(identity_size))),
                              boost::none, boost::none);
  auto put_result(data_manager_.HandlePut(from, data));
  EXPECT_TRUE(put_result.valid());
  auto put_response_result(data_manager_.HandlePutResponse<ImmutableData>(
           data.name(), put_result.value().at(0),
           maidsafe_error(VaultErrors::data_already_exists)));
  EXPECT_TRUE(put_response_result.valid());
  auto get_result(data_manager_.HandleGet<ImmutableData>(from, Identity(data.name()->string())));
  EXPECT_TRUE(get_result.valid());
  auto& pmid_holders(boost::get<std::vector<routing::DestinationAddress>>(get_result.value()));
  EXPECT_TRUE(std::none_of(pmid_holders.begin(), pmid_holders.end(),
                           [&](const routing::DestinationAddress& pmid_hoder) {
                             return put_result.value().at(0).first.data == pmid_hoder.first.data;
                           }));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
