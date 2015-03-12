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

#include "maidsafe/vault/data_manager/database.h"

namespace maidsafe {

namespace vault {

namespace test {

class DataManagerDatabaseTest : public testing::Test {
 public:
  DataManagerDatabaseTest() {}

 protected:
  DataManagerDatabase db_ { UniqueDbPath(*maidsafe::test::CreateTestPath("MaidSafe_db")) };
};

TEST_F(DataManagerDatabaseTest, BEH_Exist) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  std::vector<routing::Address> pmid_nodes;
  for (int index(0); index < 4; ++index)
    pmid_nodes.emplace_back(NodeId(RandomString(identity_size)));

  EXPECT_FALSE(db_.Exist<ImmutableData>(data.name()));
  db_.Put<ImmutableData>(data.name(), pmid_nodes);
  EXPECT_TRUE(db_.Exist<ImmutableData>(data.name()));
}

TEST_F(DataManagerDatabaseTest, BEH_Put) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  std::vector<routing::Address> pmid_nodes;
  for (int index(0); index < 4; ++index)
    pmid_nodes.emplace_back(NodeId(RandomString(identity_size)));

  auto pmids(db_.GetPmids<ImmutableData>(data.name()));
  EXPECT_FALSE(pmids.valid());
  db_.Put<ImmutableData>(data.name(), pmid_nodes);
  pmids = db_.GetPmids<ImmutableData>(data.name());
  EXPECT_EQ(pmids->size(), pmid_nodes.size());
}

TEST_F(DataManagerDatabaseTest, BEH_ReplacePmids) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  std::vector<routing::Address> pmid_nodes, new_pmid_nodes;
  for (int index(0); index < 4; ++index)
    pmid_nodes.emplace_back(NodeId(RandomString(identity_size)));

  for (int index(0); index < 4; ++index)
    new_pmid_nodes.emplace_back(NodeId(RandomString(identity_size)));

  db_.Put<ImmutableData>(data.name(), pmid_nodes);
  db_.ReplacePmidNodes<ImmutableData>(data.name(), new_pmid_nodes);
  auto pmids(db_.GetPmids<ImmutableData>(data.name()));
  for (const auto& pmid : *pmids)
     EXPECT_NE(std::find(new_pmid_nodes.begin(), new_pmid_nodes.end(), pmid), new_pmid_nodes.end());
}

TEST_F(DataManagerDatabaseTest, BEH_RemovePmid) {
  ImmutableData data(NonEmptyString(RandomString(1024)));
  std::vector<routing::Address> pmid_nodes;
  for (int index(0); index < 4; ++index)
    pmid_nodes.emplace_back(NodeId(RandomString(identity_size)));

  db_.Put<ImmutableData>(data.name(), pmid_nodes);
  db_.RemovePmid<ImmutableData>(data.name(),
                                routing::DestinationAddress(
                                    routing::Destination(pmid_nodes.at(0)), boost::none));
  auto pmids(db_.GetPmids<ImmutableData>(data.name()).value());
  EXPECT_EQ(std::find(pmids.begin(), pmids.end(), pmid_nodes.at(0)), pmids.end());
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
