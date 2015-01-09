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

#ifndef MAIDSAFE_VAULT_TESTS_HYBRID_NETWORK_H_
#define MAIDSAFE_VAULT_TESTS_HYBRID_NETWORK_H_

#include "maidsafe/common/test.h"

#include "maidsafe/routing/tests/routing_network.h"

#include "maidsafe/vault/vault.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

class HybridNetwork : public routing::test::GenericNetwork {
 public:
  typedef std::shared_ptr<Vault> VaultPtr;

  HybridNetwork();

  virtual void SetUp();
  virtual void TearDown();

  bool AddVault();

 protected:
  boost::filesystem::path vault_dir_;
  std::vector<passport::PublicPmid> public_pmids_;
  std::vector<VaultPtr> vaults_;
};

class HybridEnvironment : public testing::Environment {
 public:
  HybridEnvironment() {}

  void SetUp() override {
    try {
      g_env_ = std::make_shared<HybridNetwork>();
      g_env_->SetUp();
    }
    catch (const std::exception& e) {
      GTEST_FAIL() << e.what();
    }
  }

  void TearDown() override { g_env_->TearDown(); }

  static std::shared_ptr<HybridNetwork> g_environment() { return g_env_; }

 public:
  static std::shared_ptr<HybridNetwork> g_env_;
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_HYBRID_NETWORK_H_
