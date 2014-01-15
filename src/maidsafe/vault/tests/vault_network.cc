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
      pmids_(), chunk_store_path_(fs::unique_path((fs::temp_directory_path()))),
      network_size_(kNetworkSize) {
  asio_service_.Start();
  for (size_t index(0); index < network_size_ + 2; ++index) {
    auto pmid(MakePmid());
    pmids_.push_back(pmid);
    public_pmids_.push_back(passport::PublicPmid(pmid));
  }
}

void VaultNetwork::Bootstrap() {
  std::cout << "Creating zero state routing network..." << std::endl;
  routing::NodeInfo node_info1(MakeNodeInfo(pmids_[0])),
                    node_info2(MakeNodeInfo(pmids_[1]));
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
  routing::Routing routing1(pmids_[0]), routing2(pmids_[1]);

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
    endpoints_.erase(std::begin(endpoints_), std::begin(endpoints_) + 1);
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
    vaults_.emplace_back(new Vault(pmids_[index], path, [](const boost::asio::ip::udp::endpoint&) {},
                                   public_pmids_, endpoints_));
    LOG(kSuccess) << "vault joined: " << index;
  }
  catch (const std::exception& ex) {
    LOG(kError) << "Failed to start vault: " << index << ex.what();
  }
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

