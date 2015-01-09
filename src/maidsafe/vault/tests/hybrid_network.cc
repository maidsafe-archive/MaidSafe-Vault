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

#include "maidsafe/vault/tests/hybrid_network.h"

#include <algorithm>
#include <string>

#include "maidsafe/common/test.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/visualiser_log.h"

#include "maidsafe/passport/passport.h"

#include "maidsafe/vault/tests/tests_utils.h"

#include "maidsafe/vault_manager/vault_config.h"

namespace maidsafe {

namespace vault {

namespace test {

std::shared_ptr<HybridNetwork> HybridEnvironment::g_env_ = std::shared_ptr<HybridNetwork>();

HybridNetwork::HybridNetwork()
     : routing::test::GenericNetwork(), vault_dir_(fs::unique_path((fs::temp_directory_path()))),
       public_pmids_(), vaults_() {}

void HybridNetwork::SetUp() {
  routing::test::GenericNetwork::SetUp();
  public_pmids_.emplace_back(passport::PublicPmid(zero_states_pmids().at(0)));
  public_pmids_.emplace_back(passport::PublicPmid(zero_states_pmids().at(1)));
  for (int index(0); index < 16; ++index) {
    passport::PmidAndSigner pmid_and_signer(passport::CreatePmidAndSigner());
    AddNode(pmid_and_signer.first);
    public_pmids_.emplace_back(passport::PublicPmid(pmid_and_signer.first));
  }
}

void HybridNetwork::TearDown() {
  Sleep(std::chrono::microseconds(100));
  routing::test::GenericNetwork::TearDown();
}

bool HybridNetwork::AddVault() {
  passport::PmidAndSigner pmid_and_signer(passport::CreatePmidAndSigner());
  AddPublicKey(NodeId(pmid_and_signer.first.name()->string()),
                      pmid_and_signer.first.public_key());
  std::string path_str("vault" + RandomAlphaNumericString(6));
  auto vault_root_dir(vault_dir_ / path_str);
  fs::create_directory(vault_root_dir);
  try {
    vault_manager::VaultConfig vault_config(pmid_and_signer.first, vault_root_dir,
                                            DiskUsage(1000000000));
    vault_config.test_config.public_pmid_list = public_pmids_;
    vaults_.emplace_back(new Vault(vault_config));
    LOG(kSuccess) << "vault joined: " << vaults_.size()
                  << " id: " << NodeId(pmid_and_signer.first.name()->string());
  }
  catch (const std::exception& ex) {
    LOG(kError) << "vault failed to join: " << vaults_.size()
                << " because: " << boost::diagnostic_information(ex);
    return false;
  }
  for (auto& vault : vaults_)
    vault->AddPublicPmid(passport::PublicPmid(pmid_and_signer.first));
  public_pmids_.emplace_back(passport::PublicPmid(pmid_and_signer.first));
  Sleep(std::chrono::seconds(2));
  return true;
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
