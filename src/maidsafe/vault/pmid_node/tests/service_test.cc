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

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {


class PmidNodeServiceTest  : public testing::Test {
 public:
  PmidNodeServiceTest() :
      pmid_(MakePmid()),
      kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
      vault_root_dir_(*kTestRoot_ / RandomAlphaNumericString(8)),
      routing_(pmid_),
      data_getter_(asio_service_, routing_, std::vector<passport::PublicPmid>()),
      pmid_node_service_(pmid_, routing_, data_getter_, vault_root_dir_),
      asio_service_(2) {
    boost::filesystem::create_directory(vault_root_dir_);
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

TEST_F(PmidNodeServiceTest, BEH_PutRequestFromPmidManager) {}

TEST_F(PmidNodeServiceTest, BEH_GetRequestFromDataManager) {}

TEST_F(PmidNodeServiceTest, BEH_IntegrityCheckRequestFromDataManager) {}

TEST_F(PmidNodeServiceTest, BEH_DeleteRequestFromPmidManager) {}

TEST_F(PmidNodeServiceTest, BEH_GetPmidAccountResponseFromPmidManager) {}


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
