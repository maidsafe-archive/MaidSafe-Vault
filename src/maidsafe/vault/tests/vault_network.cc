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

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

VaultNetwork::VaultNetwork()
    : asio_service_(4), mutex_(), bootstrap_condition_(), network_up_condition_(),
      bootstrap_done_(false), network_up_(false), vaults_(), endpoints_(), public_pmids_(),
      key_chanins_(kNetworkSize + 2),
      chunk_store_path_(fs::unique_path((fs::temp_directory_path()))), network_size_(kNetworkSize) {
  asio_service_.Start();
  for (const auto& key : key_chanins_.keys)
    public_pmids_.push_back(passport::PublicPmid(key.pmid));
}

void VaultNetwork::Bootstrap() {
  std::cout << "Creating zero state routing network..." << std::endl;
  routing::NodeInfo node_info1(MakeNodeInfo(key_chanins_.keys[0].pmid)),
                    node_info2(MakeNodeInfo(key_chanins_.keys[1].pmid));
  routing::Functors functors1, functors2;
  functors1.request_public_key = functors2.request_public_key  = [&, this](
      NodeId node_id, const routing::GivePublicKeyFunctor& give_key) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    auto itr(std::find_if(std::begin(this->public_pmids_), std::end(this->public_pmids_),
                          [node_id](const passport::PublicPmid& pmid) {
                            return pmid.name()->string() == node_id.string();
                          }));
    assert(itr != std::end(this->public_pmids_));
    give_key(itr->public_key());
  };

  functors1.typed_message_and_caching.group_to_group.message_received =
      functors2.typed_message_and_caching.group_to_group.message_received =
      [&](const routing::GroupToGroupMessage&) {};
  functors1.typed_message_and_caching.group_to_single.message_received =
      functors2.typed_message_and_caching.group_to_single.message_received =
      [&](const routing::GroupToSingleMessage&) {};
  functors1.typed_message_and_caching.single_to_group.message_received =
      functors2.typed_message_and_caching.single_to_group.message_received =
      [&](const routing::SingleToGroupMessage&) {};
  functors1.typed_message_and_caching.single_to_single.message_received =
      functors2.typed_message_and_caching.single_to_single.message_received =
      [&](const routing::SingleToSingleMessage&) {};

  endpoints_.push_back(boost::asio::ip::udp::endpoint(GetLocalIp(),
                       maidsafe::test::GetRandomPort()));
  endpoints_.push_back(boost::asio::ip::udp::endpoint(GetLocalIp(),
                       maidsafe::test::GetRandomPort()));
  routing::Routing routing1(key_chanins_.keys[0].pmid), routing2(key_chanins_.keys[1].pmid);

  auto a1 = std::async(std::launch::async, [&, this] {
    return routing1.ZeroStateJoin(functors1, endpoints_[0], endpoints_[1], node_info2);
  });
  auto a2 = std::async(std::launch::async, [&, this] {
    return routing2.ZeroStateJoin(functors2, endpoints_[1], endpoints_[0], node_info1);
  });
  if (a1.get() != 0 || a2.get() != 0) {
    LOG(kError) << "SetupNetwork - Could not start bootstrap nodes.";
    ThrowError(RoutingErrors::not_connected);
  }
  bootstrap_done_ = true;
  bootstrap_condition_.notify_one();
  // just wait till process receives termination signal
  LOG(kInfo) << "Bootstrap nodes are running"  << "Endpoints: " << endpoints_[0]
             << " and " << endpoints_[1];
  {
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    assert(network_up_condition_.wait_for(lock, std::chrono::seconds(300),
                                          [this]() {
                                            return this->network_up_;
                                          }));
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    endpoints_.clear();
    boost::asio::ip::udp::endpoint live;
    live.address(maidsafe::GetLocalIp());
    live.port(5483);
    endpoints_.push_back(live);
  }
}

void VaultNetwork::SetUp() {
  auto bootstrap = std::async(std::launch::async, [&, this] {
    this->Bootstrap();
  });
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  assert(this->bootstrap_condition_.wait_for(lock, std::chrono::seconds(5),
                                             [this]() {
                                               return this->bootstrap_done_;
                                             }));
  LOG(kVerbose) << "Starting vaults...";
  std::vector<std::future<void>> vaults;
  for (size_t index(2); index < network_size_ + 2; ++index) {
    vaults.push_back(std::async(std::launch::async, [index, this] { this->Create(index); }));
    Sleep(std::chrono::seconds(std::min(index / 10 + 1, size_t(3))));
  }

  this->network_up_ = true;
  this->network_up_condition_.notify_one();
  bootstrap.get();
  for (size_t index(0); index < network_size_; ++index) {
    try {
      vaults[index].get();
    }
    catch (const std::exception& e) {
      LOG(kError) << "Exception getting future from creating vault " << index << ": " << e.what();
    }
    LOG(kVerbose) << index << " returns.";
  }
  LOG(kVerbose) << "Network is up...";
}

void VaultNetwork::TearDown() {
  while (vaults_.size() > 0) {
    vaults_.erase(vaults_.begin());
    Sleep(std::chrono::milliseconds(200));
  }
}

void VaultNetwork::Create(size_t index) {
  std::string path_str("vault" + RandomAlphaNumericString(6));
  auto path(chunk_store_path_/path_str);
  fs::create_directory(path);
  try {
    vaults_.emplace_back(new Vault(key_chanins_.keys[index].pmid, path,
                                   [](const boost::asio::ip::udp::endpoint&) {}, public_pmids_,
                                   endpoints_));
    LOG(kSuccess) << "vault joined: " << index;
  }
  catch (const std::exception& ex) {
    EXPECT_TRUE(false) << "Failed to start vault: " << index << ex.what();
  }
}

void VaultNetwork::Add() {
  key_chanins_.Add();
  Create(key_chanins_.keys.size() - 1);
}


Client::Client(const passport::detail::AnmaidToPmid& keys,
               const std::vector<UdpEndpoint>& endpoints,
               const std::vector<passport::PublicPmid>& public_pmids)
    : asio_service_(2), functors_(), routing_(keys.maid), nfs_(),
      data_getter_(asio_service_, routing_, public_pmids) {
  nfs_.reset(new nfs_client::MaidNodeNfs(
      asio_service_, routing_, passport::PublicPmid::Name(Identity(keys.pmid.name().value))));
  {
    auto future(RoutingJoin(endpoints));
    auto status(future.wait_for(std::chrono::seconds(10)));
    if (status == std::future_status::timeout || !future.get()) {
      LOG(kError) << "can't join routing network";
      ThrowError(RoutingErrors::not_connected);
    }
    LOG(kInfo) << "Client node joined routing network";
  }
  {
    passport::PublicMaid public_maid(keys.maid);
    passport::PublicAnmaid public_anmaid(keys.anmaid);
    auto future(nfs_->CreateAccount(nfs_vault::AccountCreation(public_maid, public_anmaid)));
    auto status(future.wait_for(boost::chrono::seconds(10)));
    if (status == boost::future_status::timeout) {
      LOG(kError) << "can't create account";
      ThrowError(VaultErrors::failed_to_handle_request);
    }
    // waiting for syncs resolved
    boost::this_thread::sleep_for(boost::chrono::seconds(2));
    LOG(kVerbose) << "Account created for maid " << HexSubstr(public_maid.name()->string());
  }
//  if (register_pmid_for_client) {
//    {
//      client_nfs_->RegisterPmid(nfs_vault::PmidRegistration(key_chain.maid, key_chain.pmid, false));
//      boost::this_thread::sleep_for(boost::chrono::seconds(5));
//      auto future(client_nfs_->GetPmidHealth(pmid_name));
//      auto status(future.wait_for(boost::chrono::seconds(10)));
//      if (status == boost::future_status::timeout) {
//        LOG(kError) << "can't fetch pmid health";
//        ThrowError(VaultErrors::failed_to_handle_request);
//      }
//      std::cout << "The fetched PmidHealth for pmid_name " << HexSubstr(pmid_name.value.string())
//                << " is " << future.get() << std::endl;
//    }
//    // waiting for the GetPmidHealth updating corresponding accounts
//    boost::this_thread::sleep_for(boost::chrono::seconds(5));
//    LOG(kInfo) << "Pmid Registered created for the client node to store chunks";
//  }
}

std::future<bool> Client::RoutingJoin(const std::vector<UdpEndpoint>& peer_endpoints) {
  std::once_flag join_promise_set_flag;
  std::shared_ptr<std::promise<bool>> join_promise(std::make_shared<std::promise<bool>>());
  functors_.network_status = [&join_promise_set_flag, join_promise](int result) {
    std::cout << "Network health: " << result << std::endl;
    std::call_once(join_promise_set_flag, [join_promise, &result] {
      try {
        join_promise->set_value(result > -1);
      } catch (...) {
      }
    });
  };
  functors_.typed_message_and_caching.group_to_group.message_received =
      [&](const routing::GroupToGroupMessage &msg) { nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.group_to_single.message_received =
      [&](const routing::GroupToSingleMessage &msg) { nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.single_to_group.message_received =
      [&](const routing::SingleToGroupMessage &msg) { nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.single_to_single.message_received =
      [&](const routing::SingleToSingleMessage &msg) { nfs_->HandleMessage(msg); };
  functors_.request_public_key =
      [&](const NodeId & node_id, const routing::GivePublicKeyFunctor& give_key) {
        OnPublicKeyRequested(node_id, give_key); };
  routing_.Join(functors_, peer_endpoints);
  return std::move(join_promise->get_future());
}

void Client::OnPublicKeyRequested(const NodeId& node_id,
                                  const routing::GivePublicKeyFunctor& give_key) {
  asio_service_.service().post([=] {
                                 passport::PublicPmid::Name name(Identity(node_id.string()));
                                 try {
                                   auto future(data_getter_.Get(name));
                                   give_key(future.get().public_key());
                                 } catch (const std::exception& ex) {
                                   LOG(kError) << "Failed to get key for " << DebugId(name)
                                               << " : " << ex.what();
                                 }
                               });
}

KeyChain::KeyChain(size_t size) {
  while (size-- > 0)
    Add();
}

void KeyChain::Add() {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  keys.push_back(passport::detail::AnmaidToPmid(anmaid, maid, pmid));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

