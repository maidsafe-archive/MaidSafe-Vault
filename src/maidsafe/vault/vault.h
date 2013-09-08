/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_VAULT_H_
#define MAIDSAFE_VAULT_VAULT_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/nfs/service.h"

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/version_manager/service.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/demultiplexer.h"

namespace maidsafe {

namespace vault {

class Vault {
 public:
  // pmids_from_file must only be non-empty for zero-state network.
  Vault(const passport::Pmid& pmid,
        const boost::filesystem::path& vault_root_dir,
        std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint,
        const std::vector<passport::PublicPmid>& pmids_from_file =
            std::vector<passport::PublicPmid>(),
        const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints =
            std::vector<boost::asio::ip::udp::endpoint>());
  ~Vault();  // must issue StopSending() to all identity objects (MM etc.)
             // Then ensure routing is destroyed next then all others in any order at this time
 private:
  void InitRouting(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
  routing::Functors InitialiseRoutingCallbacks();
  template<typename T>
  void OnMessageReceived(const T& message);
  void OnNetworkStatusChange(const int& network_health);
  void DoOnNetworkStatusChange(const int& network_health);
  void OnPublicKeyRequested(const NodeId &node_id, const routing::GivePublicKeyFunctor &give_key);
  void DoOnPublicKeyRequested(const NodeId &node_id, const routing::GivePublicKeyFunctor &give_key);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void OnMatrixChanged(std::shared_ptr<routing::MatrixChange> matrix_change);
  template<typename T>
  bool OnGetFromCache(const T& message);
  template<typename T>
  void OnStoreInCache(const std::string& message);
  void OnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint);

  std::mutex network_health_mutex_;
  std::condition_variable network_health_condition_variable_;
  int network_health_;
  std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint_;
  std::unique_ptr<routing::Routing> routing_;
  nfs::PublicKeyGetter public_key_getter_;
  nfs::Service<MaidManagerService> maid_manager_service_;
  nfs::Service<VersionManagerService> version_manager_service_;
  nfs::Service<DataManagerService> data_manager_service_;
  nfs::Service<PmidManagerService> pmid_manager_service_;
  nfs::Service<PmidNodeService> pmid_node_service_;
  Demultiplexer demux_;
  AsioService asio_service_;
};

template<typename T>
void Vault::OnMessageReceived(const T& message) {
  asio_service_.service().post([=] { demux_.HandleMessage(message); });
}

template<typename T>
bool Vault::OnGetFromCache(const T& message) {  // Need to be on routing's thread
  return demux_.GetFromCache(message);
}

template<typename T>
void Vault::OnStoreInCache(const T& message) {
  asio_service_.service().post([=] { demux_.StoreInCache(message) });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
