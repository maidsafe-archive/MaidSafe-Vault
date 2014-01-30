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

#ifndef MAIDSAFE_WIN32
#include <ulimit.h>
#endif

#include <algorithm>

#include "maidsafe/common/test.h"
#include "maidsafe/common/log.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

PublicKeyGetter Client::public_key_getter_;

void PublicKeyGetter::operator()(const NodeId& node_id,
                                 const routing::GivePublicKeyFunctor& give_key,
                                 const std::vector<passport::PublicPmid>& public_pmids) {
  passport::PublicPmid::Name name(Identity(node_id.string()));
  std::lock_guard<std::mutex> lock(mutex_);
  if (!public_pmids.empty()) {
    LOG(kVerbose) << "Local list contains " << public_pmids.size() << " pmids";
    try {
      auto itr(std::find_if(
               std::begin(public_pmids), std::end(public_pmids),
               [&name](const passport::PublicPmid& pmid) {
                 return pmid.name() == name;
               }));
      if (itr == std::end(public_pmids)) {
        LOG(kWarning) << "PublicPmid not found locally" << HexSubstr(name.value) << " from local";
      } else {
        LOG(kVerbose) << "Success in retreiving PublicPmid locally" << HexSubstr(name.value);
        give_key((*itr).public_key());
        return;
      }
    } catch (...) {
      LOG(kError) << "Failed to get PublicPmid " << HexSubstr(name.value) << " locally";
    }
  }
}

VaultNetwork::VaultNetwork()
    : asio_service_(2), mutex_(), bootstrap_condition_(), network_up_condition_(),
      bootstrap_done_(false), network_up_(false), vaults_(), clients_(), endpoints_(),
      public_pmids_(), key_chains_(kNetworkSize + 2),
      chunk_store_path_(fs::unique_path((fs::temp_directory_path()))), network_size_(kNetworkSize)
#ifndef MAIDSAFE_WIN32
      , kUlimitFileSize([]()->long {
                          long current_size(ulimit(UL_GETFSIZE));
                          if (current_size < kLimitsFiles)
                            ulimit(UL_SETFSIZE, kLimitsFiles);
                          return current_size;
                        }())
#endif
{
  for (const auto& key : key_chains_.keys)
    public_pmids_.push_back(passport::PublicPmid(key.pmid));
}

void VaultNetwork::Bootstrap() {
  LOG(kVerbose) << "Creating zero state routing network...";
  routing::NodeInfo node_info1(MakeNodeInfo(key_chains_.keys[0].pmid)),
                    node_info2(MakeNodeInfo(key_chains_.keys[1].pmid));
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
  functors1.typed_message_and_caching.single_to_group_relay.message_received =
      functors2.typed_message_and_caching.single_to_group_relay.message_received =
      [&](const routing::SingleToGroupRelayMessage&) {};

  endpoints_.push_back(boost::asio::ip::udp::endpoint(GetLocalIp(),
                       maidsafe::test::GetRandomPort()));
  endpoints_.push_back(boost::asio::ip::udp::endpoint(GetLocalIp(),
                       maidsafe::test::GetRandomPort()));
  routing::Routing routing1(key_chains_.keys[0].pmid), routing2(key_chains_.keys[1].pmid);

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
    endpoints_.push_back(boost::asio::ip::udp::endpoint(GetLocalIp(), 5483));
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
  std::vector<std::future<bool>> futures;
  for (size_t index(2); index < network_size_ + 2; ++index) {
    futures.push_back(std::async(std::launch::async,
                      [index, this] {
                        return this->Create(index);
                      }));
    Sleep(std::chrono::seconds(std::min(index / 10 + 1, size_t(3))));
  }

  this->network_up_ = true;
  this->network_up_condition_.notify_one();
  bootstrap.get();
  for (size_t index(0); index < network_size_; ++index) {
    try {
      EXPECT_TRUE(futures[index].get());
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
#ifndef MAIDSAFE_WIN32
  ulimit(UL_SETFSIZE, kUlimitFileSize);
#endif
}

bool VaultNetwork::Create(size_t index) {
  std::string path_str("vault" + RandomAlphaNumericString(6));
  auto path(chunk_store_path_/path_str);
  fs::create_directory(path);
  try {
    vaults_.emplace_back(new Vault(key_chains_.keys[index].pmid, path,
                                   [](const boost::asio::ip::udp::endpoint&) {}, public_pmids_,
                                   endpoints_));
    LOG(kSuccess) << "vault joined: " << index;
    return true;
  }
  catch (const std::exception& ex) {
    LOG(kError) << "vault failed to join: " << index << " because: " << ex.what();
    return false;
  }
}

bool VaultNetwork::Add() {
  auto node_keys(key_chains_.Add());
  public_pmids_.push_back(passport::PublicPmid(node_keys.pmid));
  for (size_t index(0); index < vaults_.size(); ++index) {
    vaults_[index]->AddPublicPmid(passport::PublicPmid(node_keys.pmid));
  }
  auto future(std::async(std::launch::async,
                         [this] {
                           return this->Create(this->key_chains_.keys.size() -1);
                         }));
  Sleep(std::chrono::seconds(std::min(vaults_.size() / 10 + 1, size_t(3))));
  try {
    return future.get();
  }
  catch (const std::exception& e) {
    LOG(kError) << "Exception getting future from creating vault " << e.what();
    return false;
  }
}

bool VaultNetwork::AddClient(bool register_pmid) {
  passport::detail::AnmaidToPmid node_keys;
  if (register_pmid) {
    Add();
    node_keys = *key_chains_.keys.rbegin();
  } else {
    node_keys = key_chains_.Add();
    public_pmids_.push_back(passport::PublicPmid(node_keys.pmid));
  }
  try {
    clients_.emplace_back(new Client(node_keys, endpoints_, public_pmids_, register_pmid));
    return true;
  }
  catch (...) {
    return false;
  }
}

Client::Client(const passport::detail::AnmaidToPmid& keys,
               const std::vector<UdpEndpoint>& endpoints,
               std::vector<passport::PublicPmid>& public_pmids,
               bool register_pmid_for_client)
    : asio_service_(2), functors_(), routing_(keys.maid), nfs_(),
      data_getter_(asio_service_, routing_),
      public_pmids_(public_pmids) {
  nfs_.reset(new nfs_client::MaidNodeNfs(
      asio_service_, routing_, passport::PublicPmid::Name(Identity(keys.pmid.name().value))));
  {
    auto future(RoutingJoin(endpoints));
    auto status(future.wait_for(std::chrono::seconds(10)));
    if (status == std::future_status::timeout || !future.get()) {
      LOG(kError) << "can't join routing network";
      ThrowError(RoutingErrors::not_connected);
    }
    LOG(kSuccess) << "Client node joined routing network";
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
    LOG(kSuccess) << "Account created for maid " << HexSubstr(public_maid.name()->string());
  }
  {
    try {
      nfs_->Put(passport::PublicPmid(keys.pmid));
    }
    catch (...) {
      ThrowError(CommonErrors::unknown);
    }
    Sleep(std::chrono::seconds(2));
  }
  if (register_pmid_for_client) {
    {
      nfs_->RegisterPmid(nfs_vault::PmidRegistration(keys.maid, keys.pmid, false));
      boost::this_thread::sleep_for(boost::chrono::seconds(5));
      passport::PublicPmid::Name pmid_name(Identity(keys.pmid.name().value));
      auto future(nfs_->GetPmidHealth(pmid_name));
      auto status(future.wait_for(boost::chrono::seconds(10)));
      if (status == boost::future_status::timeout) {
        LOG(kError) << "can't fetch pmid health";
        ThrowError(VaultErrors::failed_to_handle_request);
      }
      LOG(kVerbose) << "The fetched PmidHealth for pmid_name "
                    << HexSubstr(pmid_name.value.string()) << " is " << future.get();
    }
    // waiting for the GetPmidHealth updating corresponding accounts
    boost::this_thread::sleep_for(boost::chrono::seconds(5));
    LOG(kSuccess) << "Pmid Registered created for the client node to store chunks";
  }
}

std::future<bool> Client::RoutingJoin(const std::vector<UdpEndpoint>& peer_endpoints) {
  std::once_flag join_promise_set_flag;
  std::shared_ptr<std::promise<bool>> join_promise(std::make_shared<std::promise<bool>>());
  functors_.network_status = [&join_promise_set_flag, join_promise](int result) {
    LOG(kVerbose) << "Network health: " << result;
    if (result == 100)
      std::call_once(join_promise_set_flag, [join_promise] {
                                              try {
                                                join_promise->set_value(true);
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
//  functors_.typed_message_and_caching.single_to_group_relay.message_received =
//      [&](const routing::SingleToGroupRelayMessage &msg) { nfs_->HandleMessage(msg); };
  functors_.request_public_key =
      [&, this](const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key) {
        public_key_getter_(node_id, give_key, public_pmids_);
      };
  routing_.Join(functors_, peer_endpoints);
  return std::move(join_promise->get_future());
}

KeyChain::KeyChain(size_t size) {
  while (size-- > 0)
    Add();
}

passport::detail::AnmaidToPmid KeyChain::Add() {
  passport::Anmaid anmaid;
  passport::Maid maid(anmaid);
  passport::Pmid pmid(maid);
  passport::detail::AnmaidToPmid node_keys(anmaid, maid, pmid);
  keys.push_back(node_keys);
  return node_keys;
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

