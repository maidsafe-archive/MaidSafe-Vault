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

#ifndef MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_
#define MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/nfs/client/maid_node_nfs.h"
#include "maidsafe/vault/vault.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace test {

typedef boost::asio::ip::udp::endpoint UdpEndpoint;
const int kNetworkSize(1);
const int kClientsSize(2);

#ifndef MAIDSAFE_WIN32
const int kLimitsFiles(2048);
#endif

class VaultTest;
class CacheHandlerTest;
class VersionHandlerTest;
class PmidManagerTest;

class VaultNetwork {
 public:
  typedef std::shared_ptr<Vault> VaultPtr;
  typedef std::shared_ptr<nfs_client::MaidNodeNfs> ClientPtr;

  VaultNetwork();
  virtual ~VaultNetwork() {}
  virtual void SetUp();
  virtual void TearDown();

  bool AddVault();
  void AddClient();
  void AddClient(const passport::Maid& maid);
  void AddClient(const passport::MaidAndSigner& maid_and_signer);

  template <typename Data>
  Data Get(const typename Data::Name& data_name);

  template <typename Messagetype>
  void Send(size_t sender_index, const Messagetype& message) {
    vaults_[sender_index]->routing_->Send(message);
  }

  NodeId kNodeId(size_t index) { return vaults_[index]->routing_->kNodeId(); }

  friend class VaultTest;
  friend class CacheHandlerTest;
  friend class VersionHandlerTest;
  friend class PmidManagerTest;

 protected:
  void Bootstrap();
  bool Create(const passport::detail::Fob<passport::detail::PmidTag>& pmid);

  std::vector<VaultPtr> vaults_;
  std::vector<ClientPtr> clients_;
  std::vector<passport::PublicPmid> public_pmids_;
  fs::path vault_dir_;
  size_t network_size_;
#ifndef MAIDSAFE_WIN32
  long kUlimitFileSize;  // NOLINT
#endif
};

template <typename Data>
Data VaultNetwork::Get(const typename Data::Name& data_name) {
  assert(!clients_.empty() && "At least one client should exist to perform get operation");
  size_t index(RandomUint32() % clients_.size());
  auto future(clients_[index]->Get<typename Data::Name>(data_name));
  try {
    return future.get();
  }
  catch (const std::exception&) {
    throw;
  }
}

class VaultEnvironment : public testing::Environment {
 public:
  VaultEnvironment() {}

  void SetUp() override {
    g_env_.reset(new VaultNetwork());
    g_env_->SetUp();
  }

  void TearDown() override { g_env_->TearDown(); }

  static std::shared_ptr<VaultNetwork> g_environment() { return g_env_; }

 public:
  static std::shared_ptr<VaultNetwork> g_env_;
};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_
