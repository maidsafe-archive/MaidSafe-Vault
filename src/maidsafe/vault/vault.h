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
#include "maidsafe/nfs/public_pmid_helper.h"
#include "maidsafe/nfs/service.h"
#include "maidsafe/vault_manager/vault_config.h"

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/version_handler/service.h"
#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/demultiplexer.h"


namespace maidsafe {

namespace vault {

#ifdef TESTING
namespace test { class VaultNetwork; }
#endif

class Vault {
 public:
  explicit Vault(const vault_manager::VaultConfig& vault_config);

  ~Vault();  // must issue StopSending() to all identity objects (MM etc.)
             // Then ensure routing is destroyed next then all others in any order at this time

#ifdef TESTING
  void AddPublicPmid(const passport::PublicPmid& public_pmid);
#endif
  void Stop() {
    //     routing_->Stop();
    maid_manager_service_.Stop();
    version_handler_service_.Stop();
    data_manager_service_.Stop();
    pmid_manager_service_.Stop();
  }

 private:
#ifdef TESTING
  friend class test::VaultNetwork;
#endif
  void InitRouting();
  routing::Functors InitialiseRoutingCallbacks();
  template <typename T>
  void OnMessageReceived(const T& message);
  void OnNetworkStatusChange(int network_health);
  void DoOnNetworkStatusChange(int network_health);
  void OnPublicKeyRequested(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);
  void DoOnPublicKeyRequested(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void OnMatrixChanged(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);
  template <typename T>
  bool OnGetFromCache(const T& message);
  template <typename T>
  void OnStoreInCache(const T& message);
  void OnNewBootstrapContact(const routing::BootstrapContact& bootstrap_contact);
  template <typename Sender, typename Receiver>
  bool HandleGetFromCache(const nfs::TypeErasedMessageWrapper message, const Sender& sender,
                          const Receiver& receiver);

  std::mutex network_health_mutex_;
  std::condition_variable network_health_condition_variable_;
  int network_health_;
  AsioService asio_service_;
  std::unique_ptr<routing::Routing> routing_;
  std::vector<passport::PublicPmid> pmids_from_file_;
  nfs_client::DataGetter data_getter_;
  nfs::detail::PublicPmidHelper public_pmid_helper_;
  nfs::Service<MaidManagerService> maid_manager_service_;
  nfs::Service<VersionHandlerService> version_handler_service_;
  nfs::Service<DataManagerService> data_manager_service_;
  nfs::Service<PmidManagerService> pmid_manager_service_;
  nfs::Service<PmidNodeService> pmid_node_service_;
  nfs::Service<CacheHandlerService> cache_service_;
  Demultiplexer demux_;
  std::vector<std::future<void>> getting_keys_;
#ifdef TESTING
  std::mutex pmids_mutex_;
#endif
};

template <typename T>
void Vault::OnMessageReceived(const T& message) {
  LOG(kVerbose) << "Vault::OnMessageReceived";
  asio_service_.service().post([=] {
    LOG(kVerbose) << "Vault::OnMessageReceived invoked task in asio_service";
    demux_.HandleMessage(message);
  });
}

template <typename T>
CacheHandlerService::HandleMessageReturnType Vault::OnGetFromCache(const T& message) {
  LOG(kVerbose) << "Vault::OnGetFromCache: ";
  auto wrapper_tuple(nfs::ParseMessageWrapper(message.contents));
  return cache_service_.HandleMessage(wrapper_tuple, message.sender, message.receiver);
}

template <typename T>
void Vault::OnStoreInCache(const T& message) {
  // TODO(Team): To investigate the cost of running in new thread (as below) versus allowing
  //             the operation to continue on caller (routing) thread.
  LOG(kVerbose) << "Vault::OnStoreInCache: ";
  asio_service_.service().post([=] {
    LOG(kVerbose) << "Vault::OnStoreInCache2: ";
    auto wrapper_tuple(nfs::ParseMessageWrapper(message.contents));
    cache_service_.HandleMessage(wrapper_tuple, message.sender, message.receiver);
  });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
