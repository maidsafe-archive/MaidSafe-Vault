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
const int kNetworkSize(25);

#ifndef MAIDSAFE_WIN32
const int kLimitsFiles(2048);
#endif

struct KeyChain {
  explicit KeyChain(size_t size = 1);
  std::vector<passport::detail::AnmaidToPmid> keys;
  passport::detail::AnmaidToPmid Add();
};

class Client {
 public:
  Client(const passport::detail::AnmaidToPmid& keys, const std::vector<UdpEndpoint>& endpoints,
         const std::vector<passport::PublicPmid>& public_pmids,
         bool register_pmid_for_client = true);
  std::future<bool> RoutingJoin(const std::vector<UdpEndpoint>& peer_endpoints);

 public:
  void OnPublicKeyRequested(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);

  AsioService asio_service_;
  routing::Functors functors_;
  routing::Routing routing_;
  std::unique_ptr<nfs_client::MaidNodeNfs> nfs_;
  nfs_client::DataGetter data_getter_;
};

class VaultNetwork : public testing::Test {
 public:
  typedef std::shared_ptr<Vault> VaultPtr;
  typedef std::shared_ptr<Client> ClientPtr;

  VaultNetwork();
  virtual void SetUp();
  virtual void TearDown();
  bool Add();
  bool AddClient(bool register_pmid = true);

  template <typename Data>
  Data Get(const typename Data::Name& data_name);

 protected:
  void Bootstrap();
  bool Create(size_t index);

  AsioService asio_service_;
  std::mutex mutex_;
  std::condition_variable bootstrap_condition_, network_up_condition_;
  bool bootstrap_done_, network_up_;
  std::vector<VaultPtr> vaults_;
  std::vector<ClientPtr> clients_;
  std::vector<UdpEndpoint> endpoints_;
  std::vector<passport::PublicPmid> public_pmids_;
  KeyChain key_chains_;
  fs::path chunk_store_path_;
  size_t network_size_;
#ifndef MAIDSAFE_WIN32
  long kUlimitFileSize;
#endif
};

template <typename Data>
Data VaultNetwork::Get(const typename Data::Name& data_name) {
  assert(!clients_.empty() && "At least one client should exist to perform get operation");
  size_t index(RandomUint32() % clients_.size());
  auto future(clients_[index]->nfs_->Get<ImmutableData::Name>(data_name));
  try {
    return future.get();
  }
  catch (const std::exception& error) {
    throw error;
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TESTS_VAULT_NETWORK_H_
