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
#include "maidsafe/common/asio_service.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {

class PmidNodeServiceTest : public testing::Test {
 public:
  PmidNodeServiceTest()
      : pmid_(passport::CreatePmidAndSigner().first),
        kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_ /* / RandomAlphaNumericString(8)*/),
        routing_(pmid_),
        data_getter_(asio_service_, routing_),
        pmid_node_service_(pmid_, routing_, data_getter_, vault_root_dir_, DiskUsage(100000000)),
        asio_service_(2) {
    boost::filesystem::create_directory(vault_root_dir_);
  }

  template <typename Data>
  Data Get(const typename Data::Name& data_name) {
    return pmid_node_service_.handler_.Get<Data>(data_name);
  }

  template <typename Data>
  void Store(const Data& data) {
    pmid_node_service_.handler_.Put(data);
  }

 protected:
  passport::Pmid pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  nfs_client::DataGetter data_getter_;
  PmidNodeService pmid_node_service_;
  AsioService asio_service_;
};

TEST_F(PmidNodeServiceTest, BEH_PutRequestFromPmidManagerToPmidNode) {
  auto content(CreateContent<PutRequestFromPmidManagerToPmidNode::Contents>());
  auto put_request(CreateMessage<PutRequestFromPmidManagerToPmidNode>(content));
  auto group_source(CreateGroupSource(routing_.kNodeId()));
  EXPECT_NO_THROW(GroupSendToSingle(&pmid_node_service_, put_request, group_source,
                                    routing::SingleId(routing_.kNodeId())));
  EXPECT_NO_THROW(Get<ImmutableData>(ImmutableData::Name(content.name.raw_name)));
}

TEST_F(PmidNodeServiceTest, BEH_GetRequestFromDataManagerToPmidNode) {
  auto content(CreateContent<GetRequestFromDataManagerToPmidNode::Contents>());
  auto get_request(CreateMessage<GetRequestFromDataManagerToPmidNode>(content));
  auto group_source(CreateGroupSource(NodeId(content.raw_name.string())));
  EXPECT_NO_THROW(GroupSendToSingle(&pmid_node_service_, get_request, group_source,
                                    routing::SingleId(routing_.kNodeId())));
}

TEST_F(PmidNodeServiceTest, BEH_IntegrityCheckRequestFromDataManagerToPmidNode) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  NonEmptyString random_string(RandomString(64));
  Store(data);
  nfs_vault::DataNameAndRandomString content(data.name(), random_string);
  auto integrity_check_request(
      CreateMessage<IntegrityCheckRequestFromDataManagerToPmidNode>(content));
  EXPECT_NO_THROW(SingleSendsToSingle(&pmid_node_service_, integrity_check_request,
                                      routing::SingleSource(NodeId(NodeId::IdType::kRandomId)),
                                      routing::SingleId(routing_.kNodeId())));
}

TEST_F(PmidNodeServiceTest, BEH_DeleteRequestFromPmidManagerToPmidNode) {
  auto content(CreateContent<DeleteRequestFromPmidManagerToPmidNode::Contents>());
  auto delete_request(CreateMessage<DeleteRequestFromPmidManagerToPmidNode>(content));
  auto group_source(CreateGroupSource(routing_.kNodeId()));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  Store(data);
  EXPECT_NO_THROW(GroupSendToSingle(&pmid_node_service_, delete_request, group_source,
                                    routing::SingleId(routing_.kNodeId())));
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}

//TEST_F(PmidNodeServiceTest, BEH_GetPmidAccountResponseFromPmidManagerToPmidNode) {
//  nfs_client::ReturnCode return_code(CommonErrors::success);
//  nfs_client::DataNamesAndReturnCode content(return_code);
//  // ADD DATA BEFORE TEST TO PERMANENT DATA STORES
//  // ADD DATA NAME TO CONTENTS
//  auto pmid_account_response(
//      CreateMessage<GetPmidAccountResponseFromPmidManagerToPmidNode>(content));
//  auto group_source(CreateGroupSource(routing_.kNodeId()));
//  EXPECT_NO_THROW(GroupSendToSingle(&pmid_node_service_, pmid_account_response, group_source,
//                                    routing::SingleId(routing_.kNodeId())));
//}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
