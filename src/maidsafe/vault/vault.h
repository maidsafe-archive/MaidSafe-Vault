/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

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
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/pmid_node/pmid_node_service.h"
#include "maidsafe/vault/maid_manager/maid_manager_service.h"
#include "maidsafe/vault/metadata_manager/metadata_manager_service.h"
#include "maidsafe/vault/pmid_manager/pmid_manager_service.h"
#include "maidsafe/vault/version_manager/version_manager_service.h"
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
  void OnMessageReceived(const std::string& message,  const routing::ReplyFunctor& reply_functor);
  void OnNetworkStatusChange(const int& network_health);
  void DoOnNetworkStatusChange(const int& network_health);
  void OnPublicKeyRequested(const NodeId &node_id, const routing::GivePublicKeyFunctor &give_key);
  void DoOnPublicKeyRequested(const NodeId &node_id, const routing::GivePublicKeyFunctor &give_key);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void OnMatrixChanged(const routing::MatrixChange& matrix_change);
  bool OnGetFromCache(std::string& message);
  void OnStoreInCache(const std::string& message);
  void DoOnStoreInCache(const std::string& message);
  void OnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint);
  void DoOnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint);

  std::mutex network_health_mutex_;
  std::condition_variable network_health_condition_variable_;
  int network_health_;
  std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint_;
  Db db_;
  std::unique_ptr<routing::Routing> routing_;
  nfs::PublicKeyGetter public_key_getter_;
  MaidAccountHolderService maid_manager_service_;
  VersionManagerService version_manager_service_;
  MetadataManagerService metadata_manager_service_;
  PmidAccountHolderService pmid_manager_service_;
  DataHolderService pmid_node_;
  Demultiplexer demux_;
  AsioService asio_service_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
