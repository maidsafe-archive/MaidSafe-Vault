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

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

VaultNetwork::VaultNetwork()
    : asio_service_(4), mutex_(), bootstrap_wait_(), bootstrap_done_(false), vaults_(),
      endpoints_(), chunk_store_path_(fs::unique_path((fs::temp_directory_path()))) {}

void VaultNetwork::Bootstrap() {
  std::cout << "Creating zero state routing network..." << std::endl;
  passport::Pmid bootstrap1_pmid(MakePmid()), bootstrap2_pmid(MakePmid());
  routing::NodeInfo node_info1(MakeNodeInfo(bootstrap1_pmid)),
                    node_info2(MakeNodeInfo(bootstrap2_pmid));
  std::vector<passport::PublicPmid> public_pmids;
  public_pmids.push_back(passport::PublicPmid(bootstrap1_pmid));
  public_pmids.push_back(passport::PublicPmid(bootstrap2_pmid));
  routing::Functors functors1, functors2;
  functors1.request_public_key = [&public_pmids, this](
      NodeId /*node_id*/, const routing::GivePublicKeyFunctor& give_key) {
    give_key(public_pmids[1].public_key());
  };

  functors2.request_public_key = [&public_pmids, this](
      NodeId /*node_id*/, const routing::GivePublicKeyFunctor& give_key) {
    give_key(public_pmids[0].public_key());
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
  routing::Routing routing1(bootstrap1_pmid), routing2(bootstrap2_pmid);

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
  bootstrap_wait_.notify_one();
  // just wait till process receives termination signal
  LOG(kInfo) << "Bootstrap nodes are running"  << "Endpoints: " << endpoints_[0]
             << " and " << endpoints_[1];
  {
    std::condition_variable wait;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    wait.wait_for(lock, std::chrono::seconds(30), []() { return false; });
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    endpoints_.erase(std::begin(endpoints_), std::begin(endpoints_) + 1);
  }
}

void VaultNetwork::TearDown() {}

void VaultNetwork::Create(size_t number_of_vaults) {
  std::vector<passport::PublicPmid> pmids;
  while (number_of_vaults-- > 0) {
    auto pmid(MakePmid());
    std::string path_str("vault" + std::to_string(number_of_vaults));
    auto path(chunk_store_path_/path_str);
    LOG(kVerbose) << path.string();
    vaults_.emplace_back(new Vault(pmid, path, [](const boost::asio::ip::udp::endpoint&) {}, pmids,
                                   endpoints_));
    Sleep(std::chrono::seconds(3));
    LOG(kVerbose) << "vault joined: " << number_of_vaults;
  }
}

TEST_F(VaultNetwork, FUNC_SimplestTest) {
  auto bootstrap = std::async(std::launch::async, [&, this] {
    this->Bootstrap();
  });
  std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);
  this->bootstrap_wait_.wait_for(lock, std::chrono::seconds(5), [this]() {
                                                                  return this->bootstrap_done_;
                                                                });

  auto vaults = std::async(std::launch::async, [&, this] {
    this->Create(5);
  });
  bootstrap.get();
  vaults.get();
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

