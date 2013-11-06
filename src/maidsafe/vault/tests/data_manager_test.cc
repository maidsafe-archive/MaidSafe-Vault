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
    use of the MaidSafe Software.
*/

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {


class DataManagerServiceTest  : public testing::Test {
 public:
  DataManagerServiceTest() :
      pmid_(MakePmid()),
      routing_(pmid_),
      data_getter_(asio_service_, routing_, std::vector<passport::PublicPmid>()),
      data_manager_service_(pmid_, routing_, data_getter_),
      asio_service_(2) {}

 protected:
  passport::Pmid pmid_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  DataManagerService data_manager_service_;
  AsioService asio_service_;
};

TEST_F(DataManagerServiceTest, BEH_PutRequestFromMaidManager) {
  NodeId maid_node_id(NodeId::kRandomId), data_name_id;
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.data.name.raw_name.string());
  auto put_request(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  auto group_source(CreateGroupSource(maid_node_id));
  GroupSendToGroup(&data_manager_service_, put_request, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_puts_.GetUnresolvedActions().size(), 0);
}

TEST_F(DataManagerServiceTest, BEH_PutResponseFromPmidManager) {
  NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
  auto content(CreateContent<PutRequestFromMaidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.data.name.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_response(CreateMessage<PutRequestFromMaidManagerToDataManager>(content));
  GroupSendToGroup(&data_manager_service_, put_response, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_add_pmids_.GetUnresolvedActions().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_PutFailureFromPmidManager) {
  NodeId data_name_id, pmid_node_id(NodeId::kRandomId);
  auto content(CreateContent<PutFailureFromPmidManagerToDataManager::Contents>());
  data_name_id = NodeId(content.name.raw_name.string());
  auto group_source(CreateGroupSource(pmid_node_id));
  auto put_failure(CreateMessage<PutFailureFromPmidManagerToDataManager>(content));
  GroupSendToGroup(&data_manager_service_, put_failure, group_source,
                   routing::GroupId(data_name_id));
  EXPECT_EQ(this->data_manager_service_.sync_remove_pmids_.GetUnresolvedActions().size(), 1);
}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromMaidNode) {}

TEST_F(DataManagerServiceTest, BEH_GetRequestFromDataGetter) {}

TEST_F(DataManagerServiceTest, BEH_GetResponseFromPmidNode) {}

TEST_F(DataManagerServiceTest, BEH_PutToCacheFromDataManager) {}

TEST_F(DataManagerServiceTest, BEH_GetFromCacheFromDataManager) {}

TEST_F(DataManagerServiceTest, BEH_GetCachedResponseFromCacheHandler) {}

TEST_F(DataManagerServiceTest, BEH_DeleteRequestFromMaidManager) {}

TEST_F(DataManagerServiceTest, BEH_SynchroniseFromDataManager) {}

TEST_F(DataManagerServiceTest, BEH_SetPmidOnlineFromPmidManager) {}

TEST_F(DataManagerServiceTest, BEH_SetPmidOfflineFromPmidManager) {}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
