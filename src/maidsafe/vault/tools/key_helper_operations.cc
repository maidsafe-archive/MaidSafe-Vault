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

#include "maidsafe/vault/tools/key_helper_operations.h"

#include <csignal>
#include <future>
#include <memory>
#include <string>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/test.h"

#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {

namespace tools {

namespace {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

typedef boost::asio::ip::udp::endpoint UdpEndpoint;
typedef std::future<passport::PublicPmid> PublicPmidFuture;

// (dirvine) unused function
// nfs::Reply GetReply(const std::string& response) {
//  return nfs::Reply(nfs::Reply::serialised_type(NonEmptyString(response)));
//}

// asymm::PublicKey GetPublicKeyFromReply(const passport::PublicPmid::Name& name,
//                                       const passport::PublicPmid& pmid) {
//  passport::PublicPmid pmid(name, passport::PublicPmid::serialised_type(reply.data()));
//  return pmid.public_key();
//}

std::mutex g_mutex;
std::condition_variable g_cond_var;
bool g_ctrlc_pressed;

void CtrlCHandler(int /*signum*/) {
  //   LOG(kInfo) << " Signal received: " << signum;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_ctrlc_pressed = true;
  g_cond_var.notify_one();
}

}  // namespace

NetworkGenerator::NetworkGenerator() : asio_service_(1) {}

void NetworkGenerator::SetupBootstrapNodes(const PmidVector& all_keys) {
  std::cout << "Creating zero state routing network..." << std::endl;
  BootstrapData bootstrap_data(all_keys.at(0), all_keys.at(1));

  std::vector<passport::PublicPmid> all_public_pmids;
  all_public_pmids.reserve(all_keys.size());
  for (auto& pmid : all_keys)
    all_public_pmids.push_back(passport::PublicPmid(pmid));
  nfs_client::DataGetter public_key_getter(asio_service_, *bootstrap_data.routing1,
                                           all_public_pmids);
  routing::Functors functors1, functors2;
  std::vector<std::future<void>> getting_keys;
  functors1.request_public_key = functors2.request_public_key =
      [&public_key_getter, &all_public_pmids, &getting_keys](NodeId node_id,
           const routing::GivePublicKeyFunctor & give_key) {
        nfs::DoGetPublicKey(public_key_getter, node_id, give_key,
                            all_public_pmids, getting_keys);
      };
  functors1.typed_message_and_caching.group_to_group.message_received =
      functors2.typed_message_and_caching.group_to_group.message_received =
      [&](const routing::GroupToGroupMessage &) {};
  functors1.typed_message_and_caching.group_to_single.message_received =
      functors2.typed_message_and_caching.group_to_single.message_received =
      [&](const routing::GroupToSingleMessage &) {};
  functors1.typed_message_and_caching.single_to_group.message_received =
      functors2.typed_message_and_caching.single_to_group.message_received =
      [&](const routing::SingleToGroupMessage &) {};
  functors1.typed_message_and_caching.single_to_single.message_received =
      functors2.typed_message_and_caching.single_to_single.message_received =
      [&](const routing::SingleToSingleMessage &) {};
  functors1.typed_message_and_caching.single_to_group_relay.message_received =
      functors2.typed_message_and_caching.single_to_group_relay.message_received =
      [&](const routing::SingleToGroupRelayMessage &) {};

  endpoint1_ = UdpEndpoint(GetLocalIp(), test::GetRandomPort());
  endpoint2_ = UdpEndpoint(GetLocalIp(), test::GetRandomPort());
  auto a1 = std::async(std::launch::async, [&, this] {
    return bootstrap_data.routing1->ZeroStateJoin(functors1, endpoint1_, endpoint2_,
                                                  bootstrap_data.info2);
  });
  auto a2 = std::async(std::launch::async, [&, this] {
    return bootstrap_data.routing2->ZeroStateJoin(functors2, endpoint2_, endpoint1_,
                                                  bootstrap_data.info1);
  });
  if (a1.get() != 0 || a2.get() != 0) {
    LOG(kError) << "SetupNetwork - Could not start bootstrap nodes.";
    ThrowError(RoutingErrors::not_connected);
  }

  // just wait till process receives termination signal
  std::cout << "Bootstrap nodes are running, press Ctrl+C to terminate." << std::endl
            << "Endpoints: " << endpoint1_ << " and " << endpoint2_ << std::endl;
  signal(SIGINT, CtrlCHandler);
  std::unique_lock<std::mutex> lock(g_mutex);
  g_cond_var.wait(lock, [this] { return g_ctrlc_pressed; });  // NOLINT
  LOG(kInfo) << "Shutting down bootstrap nodes.";
}

std::vector<boost::asio::ip::udp::endpoint> NetworkGenerator::BootstrapEndpoints() const {
  std::vector<boost::asio::ip::udp::endpoint> endpoints(1, endpoint1_);
  endpoints.push_back(endpoint2_);
  return std::move(endpoints);
}

ClientTester::ClientTester(const passport::detail::AnmaidToPmid& key_chain,
                           const std::vector<UdpEndpoint>& peer_endpoints,
                           const std::vector<passport::PublicPmid>& public_pmids_from_file,
                           bool register_pmid_for_client)
    : asio_service_(2),
      key_chain_(key_chain),
      client_routing_(key_chain.maid),
      functors_(),
      client_nfs_(),
      kAllPmids_(public_pmids_from_file),
      getting_keys_(),
      call_once_(false) {
  passport::PublicPmid::Name pmid_name(Identity(key_chain.pmid.name().value));
  client_nfs_.reset(new nfs_client::MaidNodeNfs(asio_service_, client_routing_, pmid_name));
  {
    auto future(RoutingJoin(peer_endpoints));
    auto status(future.wait_for(std::chrono::seconds(10)));
    if (status == std::future_status::timeout || !future.get()) {
      LOG(kError) << "can't join routing network";
      ThrowError(RoutingErrors::not_connected);
    }
    LOG(kInfo) << "Client node joined routing network";
  }
  {
    passport::PublicMaid public_maid(key_chain.maid);
    passport::PublicAnmaid public_anmaid(key_chain.anmaid);
    auto future(client_nfs_->CreateAccount(nfs_vault::AccountCreation(public_maid,
                                                                      public_anmaid)));
    auto status(future.wait_for(boost::chrono::seconds(10)));
    if (status == boost::future_status::timeout) {
      LOG(kError) << "can't create account";
      ThrowError(VaultErrors::failed_to_handle_request);
    }
    // waiting for syncs resolved
    boost::this_thread::sleep_for(boost::chrono::seconds(2));
    std::cout << "Account created for maid " << HexSubstr(public_maid.name()->string())
              << std::endl;
  }
  // before register pmid, need to store pmid to network first
  client_nfs_->Put(passport::PublicPmid(key_chain.pmid));
  boost::this_thread::sleep_for(boost::chrono::seconds(2));

  if (register_pmid_for_client) {
    {
      client_nfs_->RegisterPmid(nfs_vault::PmidRegistration(key_chain.maid, key_chain.pmid, false));
      boost::this_thread::sleep_for(boost::chrono::seconds(5));
      auto future(client_nfs_->GetPmidHealth(pmid_name));
      auto status(future.wait_for(boost::chrono::seconds(10)));
      if (status == boost::future_status::timeout) {
        LOG(kError) << "can't fetch pmid health";
        ThrowError(VaultErrors::failed_to_handle_request);
      }
      std::cout << "The fetched PmidHealth for pmid_name " << HexSubstr(pmid_name.value.string())
                << " is " << future.get() << std::endl;
    }
    // waiting for the GetPmidHealth updating corresponding accounts
    boost::this_thread::sleep_for(boost::chrono::seconds(5));
    LOG(kInfo) << "Pmid Registered created for the client node to store chunks";
  }
}

ClientTester::~ClientTester() {}

std::future<bool> ClientTester::RoutingJoin(const std::vector<UdpEndpoint>& peer_endpoints) {
  std::once_flag join_promise_set_flag;
  std::shared_ptr<std::promise<bool>> join_promise(std::make_shared<std::promise<bool>>());
  functors_.network_status = [&join_promise_set_flag, join_promise, this](int result) {
    LOG(kVerbose) << "Network health: " << result;
    if ((result == 100) && (!call_once_)) {
          call_once_ = true;
          join_promise->set_value(true);
    }
  };
  functors_.typed_message_and_caching.group_to_group.message_received =
      [&](const routing::GroupToGroupMessage &msg) { client_nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.group_to_single.message_received =
      [&](const routing::GroupToSingleMessage &msg) { client_nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.single_to_group.message_received =
      [&](const routing::SingleToGroupMessage &msg) { client_nfs_->HandleMessage(msg); };
  functors_.typed_message_and_caching.single_to_single.message_received =
      [&](const routing::SingleToSingleMessage &msg) { client_nfs_->HandleMessage(msg); };
  functors_.request_public_key =
      [&](const NodeId & node_id, const routing::GivePublicKeyFunctor & give_key) {
        nfs::DoGetPublicKey(*client_nfs_, node_id, give_key, kAllPmids_, getting_keys_); };
  client_routing_.Join(functors_, peer_endpoints);
  return std::move(join_promise->get_future());
}

KeyStorer::KeyStorer(const passport::detail::AnmaidToPmid& key_chain,
                     const std::vector<UdpEndpoint>& peer_endpoints,
                     const std::vector<passport::PublicPmid>& public_pmids_from_file,
                     const KeyChainVector& key_chain_list)
    : ClientTester(key_chain, peer_endpoints, public_pmids_from_file, false),
      key_chain_list_(key_chain_list) {}

void KeyStorer::Store() {
  size_t failures(0);
  for (auto& keychain : key_chain_list_) {
    try {
      StoreKey(passport::PublicPmid(keychain.pmid));
      boost::this_thread::sleep_for(boost::chrono::seconds(2));
      auto pmid_future(client_nfs_->Get(passport::PublicPmid::Name(keychain.pmid.name())));
      if (EqualKeys<passport::PublicPmid>(passport::PublicPmid(keychain.pmid), pmid_future.get()))
        std::cout << "Pmid " << HexSubstr(keychain.pmid.name().value)
                  << " PublicPmidKey stored and verified " << std::endl;
    } catch (const std::exception& e) {
      std::cout << "Failed storing key chain of PMID " << HexSubstr(keychain.pmid.name().value)
                << ": " << e.what() << std::endl;
      ++failures;
    }
  }
  if (failures) {
    std::cout << "Could not store " << std::to_string(failures) << " out of "
              << std::to_string(key_chain_list_.size()) << std::endl;
    ThrowError(VaultErrors::failed_to_handle_request);
  }

  boost::this_thread::sleep_for(boost::chrono::seconds(5));
}

KeyVerifier::KeyVerifier(const passport::detail::AnmaidToPmid& key_chain,
                         const std::vector<UdpEndpoint>& peer_endpoints,
                         const std::vector<passport::PublicPmid>& public_pmids_from_file)
    : ClientTester(key_chain, peer_endpoints, public_pmids_from_file, false) {}

void KeyVerifier::Verify() {
  try {
    auto anmaid_future(client_nfs_->Get(passport::PublicAnmaid::Name(key_chain_.anmaid.name())));
    auto maid_future(client_nfs_->Get(passport::PublicMaid::Name(key_chain_.maid.name())));
    auto pmid_future(client_nfs_->Get(passport::PublicPmid::Name(key_chain_.pmid.name())));

    size_t verified_keys(0);
    if (EqualKeys<passport::PublicAnmaid>(passport::PublicAnmaid(key_chain_.anmaid),
                                          anmaid_future.get()))
      ++verified_keys;
    if (EqualKeys<passport::PublicMaid>(passport::PublicMaid(key_chain_.maid), maid_future.get()))
      ++verified_keys;
    if (EqualKeys<passport::PublicPmid>(passport::PublicPmid(key_chain_.pmid), pmid_future.get()))
      ++verified_keys;
    std::cout << "VerifyKeys - Verified all " << verified_keys << " keys.\n";
  }
  catch (const std::exception& ex) {
    LOG(kError) << "Failed to verify keys " << ex.what();
  }
}

DataChunkStorer::DataChunkStorer(const passport::detail::AnmaidToPmid& key_chain,
                                 const std::vector<UdpEndpoint>& peer_endpoints,
                                 const std::vector<passport::PublicPmid>& public_pmids_from_file,
                                 bool load_chunk_from_file)
    : ClientTester(key_chain, peer_endpoints, public_pmids_from_file, true),
      run_(false), chunk_list_() {
  if (load_chunk_from_file) {
    LOG(kVerbose) << "loading pre-generated chunks from file when constructing chunk_storer ......";
    LoadChunksFromFiles();
  }
}

void DataChunkStorer::StopTest() { run_ = false; }

void DataChunkStorer::Test(int32_t quantity) {
  LOG(kVerbose) << "testing ......";
  int32_t rounds(0);
  size_t num_chunks(0), num_store(0), num_get(0);
  while (!Done(quantity, rounds)) {
    OneChunkRun(num_chunks, num_store, num_get);
    ++rounds;
  }
  if (num_chunks != num_get) {
    LOG(kError) << "Error during storing or getting chunk. num_chunks : " << num_chunks
                << " num_stored : " << num_store << " num_get : " << num_get;
    ThrowError(CommonErrors::invalid_parameter);
  }
  boost::this_thread::sleep_for(boost::chrono::seconds(3));
}

void DataChunkStorer::TestWithDelete(int32_t quantity) {
  int32_t rounds(0);
  size_t num_chunks(0), num_store(0), num_get(0);
  while (!Done(quantity, rounds)) {
    OneChunkRunWithDelete(num_chunks, num_store, num_get);
    ++rounds;
  }
  if (num_chunks != num_get) {
    LOG(kError) << "Error during storing or getting chunk. num_chunks : " << num_chunks
                << " num_stored : " << num_store << " num_get : " << num_get;
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void DataChunkStorer::TestStoreChunk(int chunk_index) {
  StoreOneChunk(chunk_list_[chunk_index]);
  LOG(kInfo) << "Sleeping for network handling storing ... "
             << HexSubstr(chunk_list_[chunk_index].name().value);
  boost::this_thread::sleep_for(boost::chrono::seconds(5));
  if (!GetOneChunk(chunk_list_[chunk_index]))
    ThrowError(CommonErrors::invalid_parameter);
  std::cout << "Chunk "<< HexSubstr(chunk_list_[chunk_index].name().value)
            << " stored and verified" << std::endl;
}

void DataChunkStorer::TestFetchChunk(int chunk_index) {
  if (!GetOneChunk(chunk_list_[chunk_index]))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataChunkStorer::TestDeleteChunk(int chunk_index) {
  if (!DeleteOneChunk(chunk_list_[chunk_index]))
    ThrowError(CommonErrors::invalid_parameter);
  std::cout << "Chunk "<< HexSubstr(chunk_list_[chunk_index].name().value)
            << " deleted and verified" << std::endl;
}

bool DataChunkStorer::Done(int32_t quantity, int32_t rounds) const {
  return quantity < 1 ? run_.load() : rounds >= quantity;
}

void DataChunkStorer::OneChunkRun(size_t& num_chunks, size_t& num_store, size_t& num_get) {
  ImmutableData::serialised_type content(NonEmptyString(RandomString(1 << 18)));  // 256 KB
  ImmutableData::Name name(Identity(crypto::Hash<crypto::SHA512>(content.data)));
  std::cout << "Generated chunk name : " << HexSubstr(name.value) << " with content : "
            << HexSubstr(content->string()) << std::endl;
  ImmutableData chunk_data(name, content);
  ++num_chunks;

  StoreOneChunk(chunk_data);
  LOG(kInfo) << "Sleeping for network handling storing ... " << HexSubstr(name.value);
  boost::this_thread::sleep_for(boost::chrono::seconds(5));
  if (GetOneChunk(chunk_data)) {
    std::cout << "Stored chunk " << HexSubstr(name.value) << std::endl;
    ++num_store;
    ++num_get;
  } else {
    LOG(kError) << "Failed to store chunk " << HexSubstr(name.value);
    return;
  }

//  // The current client is anonymous, which incurs a 10 mins faded out for stored data
//  LOG(kInfo) << "Going to retrieve the stored chunk";
//  if (GetOneChunk(chunk_data)) {
//    LOG(kInfo) << "Got chunk " << HexSubstr(name.value);
//    ++num_get;
//  } else {
//    LOG(kError) << "Failed to store chunk " << HexSubstr(name.value);
//  }
}

void DataChunkStorer::OneChunkRunWithDelete(size_t& num_chunks, size_t& num_store,
                                            size_t& num_get) {
  ImmutableData::serialised_type content(NonEmptyString(RandomString(1 << 18)));  // 256 KB
  ImmutableData::Name name(Identity(crypto::Hash<crypto::SHA512>(content.data)));
  std::cout << "Generated chunk name : " << HexSubstr(name.value) << " with content : "
            << HexSubstr(content->string()) << std::endl;
  ImmutableData chunk_data(name, content);
  ++num_chunks;

  StoreOneChunk(chunk_data);
  LOG(kInfo) << "Sleeping for network handling storing ... " << HexSubstr(name.value);
  boost::this_thread::sleep_for(boost::chrono::seconds(5));
  if (GetOneChunk(chunk_data)) {
    std::cout << "Stored chunk " << HexSubstr(name.value) << std::endl;
    ++num_store;
    ++num_get;
  } else {
    LOG(kError) << "Failed to store chunk " << HexSubstr(name.value);
    return;
  }
  LOG(kInfo) << "Sleeping for network finalise getting ... " << HexSubstr(name.value);
  boost::this_thread::sleep_for(boost::chrono::seconds(5));
  std::cout << "Going to delete the stored chunk" << std::endl;
  if (DeleteOneChunk(chunk_data)) {
    std::cout << "Delete chunk " << HexSubstr(name.value) << std::endl;
  } else {
    LOG(kError) << "Failed to delete chunk " << HexSubstr(name.value);
    ThrowError(VaultErrors::failed_to_handle_request);
  }
}

void DataChunkStorer::StoreOneChunk(const ImmutableData& chunk_data) {
  client_nfs_->Put<ImmutableData>(chunk_data);
}

bool DataChunkStorer::GetOneChunk(const ImmutableData& chunk_data) {
  auto future = client_nfs_->Get(chunk_data.name());
  auto status(future.wait_for(boost::chrono::seconds(15)));
  if (status == boost::future_status::timeout) {
    LOG(kWarning) << "Failed when trying to get chunk " << HexSubstr(chunk_data.name().value);
    return false;
  }
  LOG(kVerbose) << "Compare fetched data";
  bool result(false);
  try {
    result = chunk_data.data() == future.get().data();
  } catch (const maidsafe_error& error) {
    LOG(kError) << "Encounter error when getting chunk : " << error.what();
  }
  return result;
}

bool DataChunkStorer::DeleteOneChunk(const ImmutableData& chunk_data) {
  LOG(kInfo) << "Deleting chunk " << HexSubstr(chunk_data.name().value) << " ...";
  client_nfs_->Delete(chunk_data.name());
  LOG(kInfo) << "Sleeping for network handling deleting ... " << HexSubstr(chunk_data.name().value);
  boost::this_thread::sleep_for(boost::chrono::seconds(5));
  LOG(kInfo) << "Going to retrieve the deleted chunk";
  return !GetOneChunk(chunk_data);
}

void DataChunkStorer::LoadChunksFromFiles() {
  fs::path store_path(boost::filesystem::temp_directory_path() / "Chunks");
  fs::directory_iterator end_itr;
  for (fs::directory_iterator itr(store_path); itr != end_itr; ++itr) {
    if (!fs::is_directory(itr->status())) {
      std::string string_content;
      ReadFile(itr->path(), &string_content);
      NonEmptyString temp(string_content);
      ImmutableData::serialised_type content(temp);
      ImmutableData::Name name(Identity(crypto::Hash<crypto::SHA512>(content.data)));
      ImmutableData chunk_data(name, content);
      chunk_list_.push_back(chunk_data);
    }
  }
}

// bool ExtendedTest(size_t chunk_set_count,
//                   const FobPairVector& all_keys,
//                   const std::vector<UdpEndpoint>& peer_endpoints) {
//   const size_t kNumClients(3);
//   assert(all_keys.size() >= 2 + kNumClients);
//   std::vector<std::unique_ptr<pd::Node>> clients;
//   std::vector<test::TestPath> client_paths;
//   std::vector<std::unique_ptr<pcs::RemoteChunkStore>> rcs;
//   for (size_t i = 2; i < 2 + kNumClients; ++i) {
//     std::cout << "Setting up client " << (i - 1) << " ..." << std::endl;
//     std::unique_ptr<pd::Node> client(new pd::Node);
//     client_paths.push_back(test::CreateTestPath("MaidSafe_Test_KeysHelper"));
//     client->set_account_name(all_keys[i].first.identity);
//     client->set_fob(all_keys[i].first);
//     if (client->Start(*client_paths.back(), peer_endpoints) != 0) {
//       std::cout << "Failed to start client " << (i - 1) << std::endl;
//       return false;
//     }
//     rcs.push_back(std::move(std::unique_ptr<pcs::RemoteChunkStore>(new pcs::RemoteChunkStore(
//         client->chunk_store(), client->chunk_manager(), client->chunk_action_authority()))));
//     clients.push_back(std::move(client));
//   }
//
//   const size_t kNumChunks(3 * chunk_set_count);
//   std::atomic<size_t> total_ops(0), succeeded_ops(0);
//   typedef std::map<pd::ChunkName,
//                    std::pair<NonEmptyString, Fob>> ChunkMap;
//   ChunkMap chunks;
//   for (size_t i = 0; i < kNumChunks; ++i) {
//     pd::ChunkName chunk_name;
//     NonEmptyString chunk_contents;
//     auto chunk_fob = clients[0]->fob();
//     switch (i % 3) {
//       case 0:  // DEF
//         chunk_contents = NonEmptyString(RandomString(1 << 18));  // 256 KB
//         chunk_name = priv::ApplyTypeToName(
//             crypto::Hash<crypto::SHA512>(chunk_contents),
//             priv::ChunkType::kDefault);
//         break;
//       case 1:  // MBO
//         pd::CreateSignedDataPayload(
//             NonEmptyString(RandomString(1 << 16)),  // 64 KB
//             chunk_fob.keys.private_key,
//             chunk_contents);
//         chunk_name = priv::ApplyTypeToName(
//                          NodeId(NodeId::kRandomId),
//                          priv::ChunkType::kModifiableByOwner);
//         break;
//       case 2:  // SIG
//         chunk_fob = pd::GenerateIdentity();
//         pd::CreateSignaturePacket(chunk_fob, chunk_name, chunk_contents);
//         break;
//     }
//     chunks[chunk_name] = std::make_pair(chunk_contents, chunk_fob);
//     LOG(kInfo) << "ExtendedTest - Generated chunk "
//                << pd::DebugChunkName(chunk_name) << " of size "
//                << BytesToBinarySiUnits(chunk_contents.string().size());
//   }
//
//   enum class RcsOp {
//     kStore = 0,
//     kModify,
//     kDelete,
//     kGet
//   };
//
//   auto do_rcs_op = [&rcs, &total_ops, &succeeded_ops](RcsOp rcs_op,
//                                                       ChunkMap::value_type& chunk,
//                                                       bool expect_success) {
//     std::mutex mutex;
//     std::condition_variable cond_var;
//     std::atomic<bool> cb_done(false), cb_result(false);
//     auto cb = [&cond_var, &cb_done, &cb_result](bool result) {
//       cb_result = result;
//       cb_done = true;
//       cond_var.notify_one();
//     };
//     bool result(false);
//     std::string action_name;
//     switch (rcs_op) {
//       case RcsOp::kStore:
//         action_name = "Storing";
//         result = rcs[0]->Store(chunk.first, chunk.second.first, cb, chunk.second.second);
//         break;
//       case RcsOp::kModify:
//         action_name = "Modifying";
//         {
//           NonEmptyString new_content;
//           pd::CreateSignedDataPayload(
//               NonEmptyString(RandomString(1 << 16)),  // 64 KB
//               chunk.second.second.keys.private_key,
//               new_content);
//           if (expect_success)
//             chunk.second.first = new_content;
//           result = rcs[0]->Modify(chunk.first, chunk.second.first, cb, chunk.second.second);
//         }
//         break;
//       case RcsOp::kDelete:
//         action_name = "Deleting";
//         result = rcs[0]->Delete(chunk.first, cb, chunk.second.second);
//         break;
//       case RcsOp::kGet:
//         action_name = "Getting";
//         result = (chunk.second.first.string() == rcs[1]->Get(chunk.first, chunk.second.second,
//                                                              false));
//         break;
//     }
//     if (rcs_op != RcsOp::kGet && result) {
//       std::unique_lock<std::mutex> lock(mutex);
//       cond_var.wait(lock, [&cb_done] { return cb_done.load(); });  // NOLINT
//       result = cb_result;
//     }
//     ++total_ops;
//     if (result == expect_success) {
//       ++succeeded_ops;
//       LOG(kSuccess) << "ExtendedTest - " << action_name << " "
//                     << pd::DebugChunkName(chunk.first) << " "
//                     << (result ? "succeeded" : "failed") << " as expected.";
//     } else {
//       LOG(kError) << "ExtendedTest - " << action_name << " "
//                   << pd::DebugChunkName(chunk.first) << " "
//                   << (result ? "succeeded" : "failed") << " unexpectedly.";
//     }
//   };
//
//   size_t prev_ops_diff(0);
//   auto const kStageDescriptions = []()->std::vector<std::string> {
//     std::vector<std::string> desc;
//     desc.push_back("storing chunks");
//     desc.push_back("retrieving chunks");
//     desc.push_back("modifying chunks");
//     desc.push_back("retrieving modified chunks");
//     desc.push_back("deleting chunks");
//     desc.push_back("retrieving deleted chunks");
//     desc.push_back("modifying non-existing chunks");
//     desc.push_back("re-storing chunks");
//     desc.push_back("retrieving chunks");
//     desc.push_back("deleting chunks");
//     return desc;
//   }();
//   for (int stage = 0; stage < 9; ++stage) {
//     std::cout << "Processing test stage " << (stage + 1) << ": " << kStageDescriptions[stage]
//               << "..." << std::endl;
//     for (auto& rcs_it : rcs)
//       rcs_it->Clear();
//     auto start_time = boost::posix_time::microsec_clock::local_time();
//     for (auto& chunk : chunks) {
//       auto chunk_type = priv::GetChunkType(chunk.first);
//       switch (stage) {
//         case 0:  // store all chunks
//           do_rcs_op(RcsOp::kStore, chunk, true);
//           break;
//         case 1:  // get all chunks to verify storage
//           do_rcs_op(RcsOp::kGet, chunk, true);
//           break;
//         case 2:  // modify all chunks, should only succeed for MBO
//           do_rcs_op(RcsOp::kModify, chunk,
//                     chunk_type == priv::ChunkType::kModifiableByOwner);
//           break;
//         case 3:  // get all chunks to verify (non-)modification
//           do_rcs_op(RcsOp::kGet, chunk, true);
//           break;
//         case 4:  // delete all chunks
//           do_rcs_op(RcsOp::kDelete, chunk, true);
//           break;
//         case 5:  // get all chunks to verify deletion
//           do_rcs_op(RcsOp::kGet, chunk, false);
//           break;
//         case 6:  // modify all (non-existing) chunks
//           do_rcs_op(RcsOp::kModify, chunk, false);
//           break;
//         case 7:  // store all chunks again, only SIG should fail due to revokation
//           do_rcs_op(RcsOp::kStore, chunk,
//                     chunk_type != priv::ChunkType::kSignaturePacket);
//           break;
//         case 8:  // get all chunks again, only SIG should fail due to revokation
//           do_rcs_op(RcsOp::kGet,
//                     chunk,
//                     chunk_type != priv::ChunkType::kSignaturePacket);
//           break;
//         case 9:  // delete all chunks to clean up
//           do_rcs_op(RcsOp::kDelete, chunk, true);
//           break;
//         default:
//           break;
//       }
//     }
//     for (auto& rcs_it : rcs)
//       rcs_it->WaitForCompletion();
//     auto end_time = boost::posix_time::microsec_clock::local_time();
//     size_t ops_diff = total_ops - succeeded_ops;
//     std::cout << "Stage " << (stage + 1) << ": " << kStageDescriptions[stage] << " - "
//               << (ops_diff == prev_ops_diff ? "SUCCEEDED" : "FAILED") << " ("
//               << (end_time - start_time) << ")" << std::endl;
//     prev_ops_diff = ops_diff;
//   }
//
//   for (auto& client : clients)
//     client->Stop();
//
//   std::cout << "Extended test completed " << succeeded_ops << " of " << total_ops
//             << " operations for " << kNumChunks << " chunks successfully." << std::endl;
//   return succeeded_ops == total_ops;
// }

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe

