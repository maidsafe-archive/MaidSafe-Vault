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

#include "maidsafe/vault/tests/vault_network.h"

#include <algorithm>
#include <string>

#include "maidsafe/common/test.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/passport/detail/fob.h"

#include "maidsafe/vault_manager/vault_config.h"

#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

std::shared_ptr<VaultNetwork> VaultEnvironment::g_env_ = std::shared_ptr<VaultNetwork>();

VaultNetwork::VaultNetwork()
    : vaults_(),
      clients_(),
      public_pmids_(),
      vault_dir_(fs::unique_path((fs::temp_directory_path()))) {}

void VaultNetwork::SetUp() {
  for (int index(0); index < kClientsSize; ++index)
    AddClient();

  for (size_t index(0); index < kNetworkSize; ++index)
    EXPECT_TRUE(AddVault());
}

void VaultNetwork::TearDown() {
  LOG(kInfo) << "VaultNetwork TearDown";
  for (auto& client : clients_)
    client->Stop();
  Sleep(std::chrono::seconds(1));
  for (auto& client : clients_)
    client.reset();
  Sleep(std::chrono::seconds(1));
  clients_.clear();
  for (auto& vault : vaults_)
    vault->Stop();
  Sleep(std::chrono::seconds(1));
  for (auto& vault : vaults_)
    vault.reset();
  vaults_.clear();
}

bool VaultNetwork::Create(const passport::detail::Fob<passport::detail::PmidTag>& pmid) {
  std::string path_str("vault" + RandomAlphaNumericString(6));
  auto vault_root_dir(vault_dir_ / path_str);
  fs::create_directory(vault_root_dir);
  try {
    LOG(kVerbose) << "vault joining: " << vaults_.size()
                  << " id: " << DebugId(NodeId(pmid.name()->string()));
    vault_manager::VaultConfig vault_config(pmid, vault_root_dir, DiskUsage(1000000000));
    vaults_.emplace_back(new Vault(vault_config));
    LOG(kSuccess) << "vault joined: " << vaults_.size()
                  << " id: " << DebugId(NodeId(pmid.name()->string()));
    public_pmids_.push_back(passport::PublicPmid(pmid));
  }
  catch (const std::exception& ex) {
    LOG(kError) << "vault failed to join: " << vaults_.size()
                << " because: " << boost::diagnostic_information(ex);
    return false;
  }
  Sleep(std::chrono::seconds(2));
  return true;
}

bool VaultNetwork::AddVault() {
  passport::PmidAndSigner pmid_and_signer(passport::CreatePmidAndSigner());
  auto future(clients_.front()->Put(passport::PublicPmid(pmid_and_signer.first)));
  try {
    future.get();
  }
  catch (const std::exception& error) {
    LOG(kVerbose) << "Failed to store pmid " << error.what();
    return false;
  }

  auto signer_future(clients_.front()->Put(passport::PublicAnpmid(pmid_and_signer.second)));
  try {
    signer_future.get();
  }
  catch (const std::exception& error) {
    LOG(kVerbose) << "Failed to store anpmid " << error.what();
    return false;
  }
  return Create(pmid_and_signer.first);
}

void VaultNetwork::AddClient() {
  passport::MaidAndSigner maid_and_signer{passport::CreateMaidAndSigner()};
  AddClient(maid_and_signer);
}

void VaultNetwork::AddClient(const passport::Maid& maid) {
  clients_.emplace_back(nfs_client::MaidClient::MakeShared(maid));
}

void VaultNetwork::AddClient(const passport::MaidAndSigner& maid_and_signer) {
  clients_.emplace_back(nfs_client::MaidClient::MakeShared(maid_and_signer));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
