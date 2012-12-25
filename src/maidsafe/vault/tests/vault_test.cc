/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/vault/vault.h"

namespace maidsafe {

namespace vault {

namespace test {

class VaultTest : public testing::Test {
 public:
  VaultTest()
    : vault_root_directory_("vault-root-directory") {
}
   protected:

  passport::Maid MakeMaid() {
    passport::Anmaid anmaid;
    return passport::Maid(anmaid);
  }

  passport::Pmid MakePmid() {
    return passport::Pmid(MakeMaid());
  }
    std::shared_ptr<Vault> vault_;
    boost::filesystem::path vault_root_directory_;
  };

TEST_F(VaultTest, BEH_Constructor) {
  passport::Pmid pmid(MakePmid());
    maidsafe::test::TestPath test_path(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault"));
    boost::filesystem::path vault_root_dir(*test_path / RandomAlphaNumericString(8));
    std::function<void(boost::asio::ip::udp::endpoint)>
        on_new_bootstrap_endpoint = [](boost::asio::ip::udp::endpoint /*ep*/) {
                                    };
    Vault vault(pmid, vault_root_dir, on_new_bootstrap_endpoint);
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
