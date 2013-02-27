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

#include "maidsafe/vault/tools/key_helper_operations.h"

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/test.h"

#include "maidsafe/routing/parameters.h"


namespace maidsafe {

namespace vault {

namespace tools {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

typedef boost::asio::ip::udp::endpoint UdpEndpoint;

nfs::Reply GetReply(const std::string& response) {
  return nfs::Reply(nfs::Reply::serialised_type(NonEmptyString(response)));
}

asymm::PublicKey GetPublicKeyFromReply(const nfs::Reply& reply) {
  return asymm::DecodeKey(asymm::EncodedPublicKey(reply.data().string()));
}

std::mutex g_mutex;
std::condition_variable g_cond_var;
bool g_ctrlc_pressed;

void CtrlCHandler(int /*signum*/) {
//   LOG(kInfo) << " Signal received: " << signum;
  std::lock_guard<std::mutex> lock(g_mutex);
  g_ctrlc_pressed = true;
  g_cond_var.notify_one();
}

NetworkGenerator::NetworkGenerator() {}

void NetworkGenerator::SetupBootstrapNodes(const PmidVector& all_pmids) {
  std::cout << "Creating zero state routing network..." << std::endl;
  BootstrapData bootstrap_data(all_pmids.at(0), all_pmids.at(1));

  std::vector<passport::PublicPmid> all_public_pmids;
  all_public_pmids.reserve(all_pmids.size());
  for (auto& pmid : all_pmids)
    all_public_pmids.push_back(passport::PublicPmid(pmid));
  nfs::PublicKeyGetter public_key_getter(*bootstrap_data.routing1, all_public_pmids);

  routing::Functors functors1, functors2;
  functors1.request_public_key = functors2.request_public_key =
      [&public_key_getter, this] (NodeId node_id, const routing::GivePublicKeyFunctor& give_key) {
        DoOnPublicKeyRequested(node_id, give_key, public_key_getter);
      };

  endpoint1_ = UdpEndpoint(GetLocalIp(), test::GetRandomPort());
  endpoint2_ = UdpEndpoint(GetLocalIp(), test::GetRandomPort());
  auto a1 = std::async(std::launch::async,
                       [&, this] {
                         return bootstrap_data.routing1->ZeroStateJoin(functors1,
                                                                       endpoint1_,
                                                                       endpoint2_,
                                                                       bootstrap_data.info2);
                       });
  auto a2 = std::async(std::launch::async,
                       [&, this] {
                         return bootstrap_data.routing2->ZeroStateJoin(functors2,
                                                                       endpoint2_,
                                                                       endpoint1_,
                                                                       bootstrap_data.info1);
                       });
  if (a1.get() != 0 || a2.get() != 0) {
    LOG(kError) << "SetupNetwork - Could not start bootstrap nodes.";
    throw std::exception();
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

void NetworkGenerator::DoOnPublicKeyRequested(const NodeId& node_id,
                                              const routing::GivePublicKeyFunctor& give_key,
                                              nfs::PublicKeyGetter& public_key_getter) {
  public_key_getter.GetKey<passport::PublicPmid>(
      passport::PublicPmid::name_type(Identity(node_id.string())),
      [give_key, node_id] (nfs::Reply reply) {
        if (reply.IsSuccess()) {
          try {
            asymm::PublicKey public_key(GetPublicKeyFromReply(reply));
            give_key(public_key);
          }
          catch(const std::exception& ex) {
            LOG(kError) << "Failed to get key for " << DebugId(node_id) << " : " << ex.what();
          }
        }
      });
}

ClientTester::ClientTester(const std::vector<UdpEndpoint>& peer_endpoints)
    : client_anmaid_(),
      client_maid_(client_anmaid_),
      client_pmid_(client_maid_),
      client_routing_(nullptr),
      functors_(),
      client_nfs_() {
  if (!RoutingJoin(peer_endpoints).get()) {
    LOG(kError) << "Failed to bootstrap anonymous node for storing keys";
    throw std::exception();
  }
  LOG(kInfo) << "Bootstrapped anonymous node to store keys";

  client_nfs_.reset(new nfs::ClientMaidNfs(client_routing_, client_maid_));
}

ClientTester::~ClientTester() {}

std::future<bool> ClientTester::RoutingJoin(const std::vector<UdpEndpoint>& peer_endpoints) {
  std::once_flag join_promise_set_flag;
  std::promise<bool> join_promise;
  functors_.network_status = [&join_promise_set_flag, &join_promise] (int result) {
                              std::call_once(join_promise_set_flag,
                                             [&join_promise, &result] {
                                               join_promise.set_value(result == 0);
                                             });
                            };
  // To allow bootstrapping off vaults on local machine
  routing::Parameters::append_local_live_port_endpoint = true;
  client_routing_.Join(functors_, peer_endpoints);
  return std::move(join_promise.get_future());
}

KeyStorer::KeyStorer(const std::vector<UdpEndpoint>& peer_endpoints)
    : ClientTester(peer_endpoints) {}

void KeyStorer::Store(const PmidVector& all_pmids) {
  auto store_keys = [this] (const passport::Pmid& pmid, std::promise<bool>& promise) {
                      passport::PublicPmid p_pmid(pmid);
                      client_nfs_->Put(p_pmid, client_pmid_.name(), callback(std::ref(promise)));
                    };

  std::vector<BoolPromise> bool_promises(all_pmids.size());
  std::vector<BoolFuture> bool_futures;
  size_t promises_index(0), error_stored_keys(0);
  for (auto& pmid : all_pmids) {  // store all PMIDs
    std::async(std::launch::async, store_keys, pmid, std::ref(bool_promises.at(promises_index)));
    bool_futures.push_back(bool_promises.at(promises_index).get_future());
  }

  for (auto& future : bool_futures) {
    if (future.has_exception() || !future.get())
      ++error_stored_keys;
  }

  if (error_stored_keys > 0) {
    LOG(kError) << "StoreKeys - Could not store " << error_stored_keys << " out of "
                << all_pmids.size() << " keys.";
    throw std::exception();
  }
  LOG(kSuccess) << "StoreKeys - Stored all " << all_pmids.size() << " keys.";
}

routing::ResponseFunctor KeyStorer::callback(std::promise<bool>& promise) {
  return [&promise] (std::string response) {
           try {
             nfs::Reply reply(GetReply(response));
             promise.set_value(reply.IsSuccess());
           }
           catch(...) {
             promise.set_exception(std::current_exception());
           }
         };
}

KeyVerifier::KeyVerifier(const std::vector<UdpEndpoint>& peer_endpoints)
    : ClientTester(peer_endpoints) {}

void KeyVerifier::Verify(const PmidVector& all_pmids) {
  auto verify_keys = [this] (const passport::Pmid& pmid, std::promise<bool>& promise) {
                       passport::PublicPmid p_pmid(pmid);
                       client_nfs_->Get<passport::PublicPmid>(p_pmid.name(),
                                                              callback(pmid, std::ref(promise)));
                     };

  std::vector<BoolFuture> futures;
  std::vector<BoolPromise> promises(all_pmids.size());
  size_t promises_index(0), verified_keys(0);
  for (auto& pmid : all_pmids) {
    std::async(std::launch::async, verify_keys, pmid, std::ref(promises.at(promises_index)));
    futures.push_back(promises.at(promises_index).get_future());
    ++promises_index;
  }

  for (auto& future : futures) {
    if (!future.has_exception() && future.get())
      ++verified_keys;
  }

  if (verified_keys < all_pmids.size()) {
    LOG(kError) << "VerifyKeys - Could only verify " << verified_keys << " out of "
                << all_pmids.size() << " keys.";
    throw std::exception();
  }
  LOG(kSuccess) << "VerifyKeys - Verified all " << verified_keys << " keys.";
}

routing::ResponseFunctor KeyVerifier::callback(const passport::Pmid& pmid,
                                               std::promise<bool>& promise) {
  return [&promise, &pmid] (std::string response) {
            nfs::Reply reply(GetReply(response));
            if (reply.IsSuccess()) {
              try {
                asymm::PublicKey public_key(GetPublicKeyFromReply(reply));
                promise.set_value(asymm::MatchingKeys(public_key,
                                                      pmid.public_key()));
              }
              catch(...) {
                LOG(kError) << "Failed to get key for " << DebugId(pmid.name());
                promise.set_exception(std::current_exception());
              }
            }
          };
}

DataChunkStorer::DataChunkStorer(const std::vector<UdpEndpoint>& peer_endpoints)
    : ClientTester(peer_endpoints),
      run_(false) {}

void DataChunkStorer::StopTest() { run_ = false; }

void DataChunkStorer::Test(int32_t quantity) {
  int32_t rounds(0);
  size_t num_chunks(0), num_store(0), num_get(0);
  while (!Done(quantity, rounds))
    OneChunkRun(num_chunks, num_store, num_get);
  if (num_chunks != num_get)
    throw std::exception();
}

bool DataChunkStorer::Done(int32_t quantity, int32_t rounds) const {
  return quantity < 1 ? run_.load() : rounds < quantity;
}

void DataChunkStorer::OneChunkRun(size_t& num_chunks, size_t& num_store, size_t& num_get) {
  ImmutableData::serialised_type content(NonEmptyString(RandomString(1 << 18)));  // 256 KB
  ImmutableData::name_type name(Identity(crypto::Hash<crypto::SHA512>(content.data)));
  ImmutableData chunk_data(name, content);
  ++num_chunks;

  if (StoreOneChunk(chunk_data)) {
    LOG(kInfo) << "Stored chunk " << HexSubstr(name.data) << std::endl;
    ++num_store;
  } else {
    LOG(kError) << "Failed to store chunk " << HexSubstr(name.data);
    return;
  }

  // The current client is anonymous, which incurs a 10 mins faded out for stored data
  LOG(kInfo) << "Going to retrieve the stored chunk";
  if (GetOneChunk(chunk_data)) {
    LOG(kInfo) << "Got chunk " << HexSubstr(name.data);
    ++num_get;
  } else {
    LOG(kError) << "Failed to store chunk " << HexSubstr(name.data);
  }
}

bool DataChunkStorer::StoreOneChunk(const ImmutableData& chunk_data) {
  std::promise<bool> store_promise;
  std::future<bool> store_future(store_promise.get_future());
  routing::ResponseFunctor cb([&store_promise] (std::string response) {
                                          try {
                                            nfs::Reply reply(GetReply(response));
                                            store_promise.set_value(reply.IsSuccess());
                                          }
                                          catch(...) {
                                            store_promise.set_exception(std::current_exception());
                                          }
                                        });
  LOG(kInfo) << "Storing chunk " << HexSubstr(chunk_data.data()) << " ...";
  client_nfs_->Put(chunk_data, client_pmid_.name(), cb);
  return store_future.get();
}

bool DataChunkStorer::GetOneChunk(const ImmutableData& chunk_data) {
  std::promise<bool> get_promise;
  std::future<bool> get_future(get_promise.get_future());
  auto equal_immutables = [] (const ImmutableData& lhs, const ImmutableData& rhs) {
                            return lhs.name().data.string() == lhs.name().data.string() &&
                                   lhs.data().string() == rhs.data().string();
                          };

  routing::ResponseFunctor cb([&chunk_data, &get_promise, &equal_immutables]
                              (std::string response) {
                                try {
                                  nfs::Reply reply(GetReply(response));
                                  ImmutableData data(reply.data());
                                  get_promise.set_value(equal_immutables(chunk_data, data));
                                }
                                catch(...) {
                                  get_promise.set_exception(std::current_exception());
                                }
                              });
  client_nfs_->Get<ImmutableData>(chunk_data.name(), cb);
  return get_future.get();
}

// bool ExtendedTest(const size_t& chunk_set_count,
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
