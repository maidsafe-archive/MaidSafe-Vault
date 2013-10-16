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
#include "maidsafe/vault/version_handler/service.h"
#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/demultiplexer.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class Vault {
 public:
  // pmids_from_file must only be non-empty for zero-state network.
  Vault(const passport::Pmid& pmid, const boost::filesystem::path& vault_root_dir,
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
  template <typename T>
  void OnMessageReceived(const T& message);
  void OnNetworkStatusChange(int network_health);
  void DoOnNetworkStatusChange(int network_health);
  void OnPublicKeyRequested(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);
  void DoOnPublicKeyRequested(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void OnMatrixChanged(std::shared_ptr<routing::MatrixChange> matrix_change);
  template <typename T>
  bool OnGetFromCache(const T& message);
  template <typename T>
  void OnStoreInCache(const T& message);
  void OnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint);
  template <typename Sender, typename Receiver>
  bool HandleGetFromCache(const nfs::TypeErasedMessageWrapper message, const Sender& sender, const Receiver& receiver);

  std::mutex network_health_mutex_;
  std::condition_variable network_health_condition_variable_;
  int network_health_;
  std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint_;
  std::unique_ptr<routing::Routing> routing_;
  nfs_client::DataGetter data_getter_;
  nfs::Service<MaidManagerService> maid_manager_service_;
  nfs::Service<VersionHandlerService> version_handler_service_;
  nfs::Service<DataManagerService> data_manager_service_;
  nfs::Service<PmidManagerService> pmid_manager_service_;
  nfs::Service<PmidNodeService> pmid_node_service_;
  CacheHandlerService cache_service_;
  Demultiplexer demux_;
  AsioService asio_service_;
};

template <typename T>
void Vault::OnMessageReceived(const T& message) {
  asio_service_.service().post([=] { demux_.HandleMessage(message); });
}

template <typename T>
bool Vault::OnGetFromCache(const T& /*message*/) {
  T::under_construction;
  return false;
}

template <>
bool Vault::OnGetFromCache(const routing::SingleToGroupMessage& message);

template <>
bool Vault::OnGetFromCache(const routing::SingleToSingleMessage& message);

template <>
bool Vault::OnGetFromCache(const routing::GroupToGroupMessage& message);

template <>
bool Vault::OnGetFromCache(const routing::GroupToSingleMessage& message);

template <typename Sender, typename Receiver>
bool Vault::HandleGetFromCache(const nfs::TypeErasedMessageWrapper wrapper_tuple,
                               const Sender& sender, const Receiver& receiver) {
//  auto source_persona(std::get<1>(wrapper_tuple).data);
  GetFromCacheMessages get_from_cache_variant;
  if (GetCacheVariant(wrapper_tuple, get_from_cache_variant)) {
     GetFromCacheVisitor<Sender, Receiver> cache_get_visitor(cache_service_, sender, receiver);
    return boost::apply_visitor(cache_get_visitor, get_from_cache_variant);
  }
  return false;
}

template <typename T>
void Vault::OnStoreInCache(const T& message) {
  asio_service_.service().post([=] {
                                 auto wrapper_tuple(nfs::ParseMessageWrapper(message.contents));
                                 cache_service_.Store(wrapper_tuple, message.sender,
                                                      message.receiver);
                               });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
