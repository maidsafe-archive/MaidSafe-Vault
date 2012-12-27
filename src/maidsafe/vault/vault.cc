/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/vault.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/node_info.h"

namespace maidsafe {

namespace vault {
Vault::Vault(passport::Pmid pmid,
             boost::filesystem::path vault_root_dir,
             std::function<void(boost::asio::ip::udp::endpoint)> on_new_bootstrap_endpoint,
             const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints)
    : network_status_mutex_(),
      on_new_bootstrap_endpoint_(on_new_bootstrap_endpoint),
      routing_(new routing::Routing(&pmid)),
      public_key_getter_(*routing_),
      maid_account_holder_(*routing_, vault_root_dir),
      meta_data_manager_(*routing_, vault_root_dir),
      pmid_account_holder_(*routing_, vault_root_dir),
      data_holder_(vault_root_dir),
      demux_(maid_account_holder_, meta_data_manager_, pmid_account_holder_, data_holder_),
      asio_service_(2) {
  asio_service_.Start();
  InitRouting(peer_endpoints);
}

Vault::~Vault() {
// call stop on all component

}

int Vault::InitRouting(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
  routing::Functors functors(InitialiseRoutingCallbacks());
  routing_->Join(functors, peer_endpoints);
  return 0;
}

routing::Functors Vault::InitialiseRoutingCallbacks() {
  routing::Functors functors;
  functors.message_received = [this] (const std::string& message,
                                      const NodeId& /*group_claim*/, // to be removed
                                      bool /*cache_lookup*/,
                                      const routing::ReplyFunctor& reply_functor) {
                                  OnMessageReceived(message, reply_functor);
                              };
  functors.network_status = [this] (const int& network_health) {
                                OnNetworkStatusChange(network_health);
                            };
  functors.close_node_replaced = [this] (const std::vector<routing::NodeInfo>& new_close_nodes) {
                                     OnCloseNodeReplaced(new_close_nodes);
                                 };
  functors.request_public_key = [this] (const NodeId& node_id,
                                        const routing::GivePublicKeyFunctor& give_key) {
                                    OnPublicKeyRequested(node_id, give_key);
                                };
  functors.new_bootstrap_endpoint = [this] (const boost::asio::ip::udp::endpoint& endpoint) {
                                        OnNewBootstrapEndpoint(endpoint);
                                    };
  functors.store_cache_data = [this] (const std::string& message) {
                                  OnStoreCacheData(message);
                              };
  functors.have_cache_data = [this] (std::string& message) {
                                 return HaveCacheData(message);
                              };
  return functors;
}

void Vault::OnMessageReceived(const std::string& message,
                              const routing::ReplyFunctor& reply_functor) {
  asio_service_.service().post([=]() { DoOnMessageReceived(message, reply_functor); });  // NOLINT (Prakash)
}

void Vault::DoOnMessageReceived(const std::string& message,
                                const routing::ReplyFunctor& reply_functor) {
  demux_.HandleMessage(message, reply_functor);
}

void Vault::OnNetworkStatusChange(const int& network_health) {
  asio_service_.service().post([=]() { DoOnNetworkStatusChange(network_health); });  // NOLINT (Prakash)
}

void Vault::DoOnNetworkStatusChange(const int& network_health) {
  if (network_health >= 0) {
    if (network_health >= network_health_)
      LOG(kVerbose) << "Init - " << DebugId(routing_->kNodeId())
                    << " - Network health is " << network_health
                    << "% (was " << network_health_ << "%)";
    else
      LOG(kWarning) << "Init - " << DebugId(routing_->kNodeId())
                    << " - Network health is " << network_health
                    << "% (was " << network_health_ << "%)";
  } else {
    LOG(kWarning) << "Init - " << DebugId(routing_->kNodeId())
                  << " - Network is down (" << network_health << ")";
  }
  network_health_ = network_health;
  // TODO (Team) : actions when network is down/up per persona
}

void Vault::OnPublicKeyRequested(const NodeId& node_id,
                                 const routing::GivePublicKeyFunctor& give_key) {
  asio_service_.service().post([=]() { DoOnPublicKeyRequested(node_id, give_key); });  // NOLINT (Prakash)
}

void Vault::DoOnPublicKeyRequested(const NodeId& node_id,
                                   const routing::GivePublicKeyFunctor& give_key) {
  public_key_getter_.HandleGetKey(node_id, give_key);
}

void Vault::OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes) {
  asio_service_.service().post([=]() { DoOnCloseNodeReplaced(new_close_nodes); });  // NOLINT (Prakash)
}

void Vault::DoOnCloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
//  pass it to all persona
}

void Vault::OnStoreCacheData(const std::string& message) {  // post/move data?
  asio_service_.service().post([=]() { DoOnStoreCacheData(message); });  // NOLINT (Prakash)
}

void Vault::DoOnStoreCacheData(const std::string& message) {
  demux_.StoreCache(message);
}

bool Vault::HaveCacheData(std::string& message) {  // Need to be on routing's thread
  return demux_.HaveCache(message);
}

void Vault::OnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint) {
  asio_service_.service().post([=]() { DoOnNewBootstrapEndpoint(endpoint); });  // NOLINT (Prakash)
}

void Vault::DoOnNewBootstrapEndpoint(const boost::asio::ip::udp::endpoint& endpoint) {
  on_new_bootstrap_endpoint_(endpoint);
}

}  // namespace vault

}  // namespace maidsafe
