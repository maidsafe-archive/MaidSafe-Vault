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

#include "maidsafe/vault/vault.h"

#include "boost/asio/ip/host_name.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/node_info.h"

#include "maidsafe/vault/parameters.h"


namespace maidsafe {

namespace vault {


Vault::Vault(const vault_manager::VaultConfig& vault_config)
    : network_health_mutex_(),
      network_health_condition_variable_(),
      network_health_(-1),
      asio_service_(2),
      routing_(new routing::Routing(vault_config.pmid)),
#ifdef TESTING
      pmids_from_file_(vault_config.test_config.public_pmid_list),
#endif
      data_getter_(asio_service_, *routing_),
      public_pmid_helper_(),
      maid_manager_service_(std::move(maidsafe::make_unique<MaidManagerService>(
          vault_config.pmid, *routing_, data_getter_, vault_config.vault_dir))),
      version_handler_service_(std::move(maidsafe::make_unique<VersionHandlerService>(
          vault_config.pmid, *routing_, vault_config.vault_dir))),
      data_manager_service_(std::move(maidsafe::make_unique<DataManagerService>(
          vault_config.pmid, *routing_, data_getter_, vault_config.vault_dir))),
      pmid_manager_service_(std::move(maidsafe::make_unique<PmidManagerService>(
          vault_config.pmid, *routing_))),
      pmid_node_service_(std::move(maidsafe::make_unique<PmidNodeService>(
          vault_config.pmid, *routing_, data_getter_, vault_config.vault_dir,
          vault_config.max_disk_usage))),
      mpid_manager_service_(std::move(maidsafe::make_unique<MpidManagerService>(
          vault_config.pmid, *routing_, vault_config.vault_dir, vault_config.max_disk_usage))),
      // FIXME need to specialise
      cache_service_(std::move(maidsafe::make_unique<CacheHandlerService>(
          *routing_, vault_config.vault_dir))),
      demux_(maid_manager_service_, version_handler_service_, data_manager_service_,
             pmid_manager_service_, pmid_node_service_, mpid_manager_service_, data_getter_),
      getting_keys_()
#ifdef TESTING
      ,
      pmids_mutex_()
#endif
{
#ifdef USE_VLOGGING
  log::Logging::Instance().InitialiseVlog(DebugId(vault_config.pmid.name()),
      vault_config.vlog_session_id, "visualiser.maidsafe.net", 8080, "/log");
# ifdef TESTING
  if (vault_config.send_hostname_to_visualiser_server)
    VLOG(VisualiserAction::kVaultStarted, Identity{ vault_config.pmid.name().value },
         boost::asio::ip::host_name());
  else
# endif
  VLOG(VisualiserAction::kVaultStarted, Identity{ vault_config.pmid.name().value });
#endif

  // TODO(Fraser#5#): 2013-03-29 - Prune all empty dirs.
  InitRouting();
}

Vault::~Vault() {
  Stop();
  asio_service_.Stop();
  routing_.reset();
}

#ifdef TESTING
void Vault::AddPublicPmid(const passport::PublicPmid& public_pmid) {
  std::lock_guard<std::mutex> lock(pmids_mutex_);
  pmids_from_file_.push_back(public_pmid);
}
#endif

void Vault::InitRouting() {
  routing::Functors functors(InitialiseRoutingCallbacks());
  routing_->Join(functors);

  std::unique_lock<std::mutex> lock(network_health_mutex_);
  network_health_condition_variable_.wait(
      lock, [this] { return network_health_ >= detail::Parameters::kMinNetworkHealth; });
}

routing::Functors Vault::InitialiseRoutingCallbacks() {
  routing::Functors functors;
  functors.typed_message_and_caching.single_to_single.message_received = [this](
      const routing::SingleToSingleMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.message_received = [this](
      const routing::SingleToGroupMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.message_received = [this](
      const routing::GroupToSingleMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.message_received = [this](
      const routing::GroupToGroupMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group_relay.message_received = [this](
      const routing::SingleToGroupRelayMessage& message) { OnMessageReceived(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_single.get_cache_data = [this](
      const routing::SingleToSingleMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.get_cache_data = [this](
      const routing::SingleToGroupMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.get_cache_data = [this](
      const routing::GroupToSingleMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.get_cache_data = [this](
      const routing::GroupToGroupMessage& message) { return OnGetFromCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_single.put_cache_data = [this](
      const routing::SingleToSingleMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.single_to_group.put_cache_data = [this](
      const routing::SingleToGroupMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_single.put_cache_data = [this](
      const routing::GroupToSingleMessage& message) { OnStoreInCache(message); };  // NOLINT
  functors.typed_message_and_caching.group_to_group.put_cache_data = [this](
      const routing::GroupToGroupMessage& message) { OnStoreInCache(message); };  // NOLINT

  functors.network_status = [this](const int& network_health) {
    OnNetworkStatusChange(network_health);
  };  // NOLINT
  functors.close_nodes_change = [this](
      std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
    OnCloseNodesChange(close_nodes_change);
  };
  functors.client_nodes_change = [this](
      std::shared_ptr<routing::ClientNodesChange> client_nodes_change) {
    OnClientNodesChange(client_nodes_change);
  };
  functors.request_public_key = [this](const NodeId& node_id,
                                       const routing::GivePublicKeyFunctor& give_key) {
    nfs::detail::DoGetPublicKey(data_getter_, node_id, give_key, pmids_from_file_,
                                public_pmid_helper_);
  };
  return functors;
}

void Vault::OnNetworkStatusChange(int network_health) {
  VLOG(VisualiserAction::kNetworkHealth, network_health);
  asio_service_.service().post([=] { DoOnNetworkStatusChange(network_health); });
}

void Vault::DoOnNetworkStatusChange(int network_health) {
  if (routing_)
    routing::UpdateNetworkHealth(network_health, network_health_, network_health_mutex_,
                                 network_health_condition_variable_, routing_->kNodeId());
  else
    LOG(kError) << "routing_ is not initialized";
  // TODO(Team) : actions when network is down/up per persona
}

void Vault::OnCloseNodesChange(std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  //   LOG(kVerbose) << "OnMatrixChanged ";
  //   close_nodes_change->Print();
  //   data_manager_service_.HandleChurnEvent(close_nodes_change);
  asio_service_.service().post([=] { maid_manager_service_.HandleChurnEvent(close_nodes_change); });
  asio_service_.service().post([=] {
    version_handler_service_.HandleChurnEvent(close_nodes_change);
  });
  asio_service_.service().post([=] { data_manager_service_.HandleChurnEvent(close_nodes_change); });
  asio_service_.service().post([=] { pmid_manager_service_.HandleChurnEvent(close_nodes_change); });
  asio_service_.service().post([=] { mpid_manager_service_.HandleChurnEvent(close_nodes_change); });
}

void Vault::OnClientNodesChange(std::shared_ptr<routing::ClientNodesChange> client_nodes_change) {
  asio_service_.service().post([=] {
    mpid_manager_service_.HandleChurnEvent(client_nodes_change);
  });
}

}  // namespace vault

}  // namespace maidsafe
