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

#include <signal.h>

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/program_options.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/config.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/routing/node_info.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing_api.h"


namespace fs = boost::filesystem;
namespace po = boost::program_options;

namespace {

typedef std::vector<maidsafe::passport::Pmid> PmidVector;

const std::string kHelperVersion = "MaidSafe Vault KeysHelper " + maidsafe::kApplicationVersion;

std::mutex mutex_;
std::condition_variable cond_var_;
bool ctrlc_pressed(false);

void CtrlCHandler(int /*signum*/) {
typedef std::vector<maidsafe::passport::Pmid> PmidVector;
//   LOG(kInfo) << " Signal received: " << signum;
  std::lock_guard<std::mutex> lock(mutex_);
  ctrlc_pressed = true;
  cond_var_.notify_one();
}

void PrintKeys(const PmidVector &all_pmids) {
  for (size_t i = 0; i < all_pmids.size(); ++i)
    std::cout << '\t' << i << "\t PMID " << maidsafe::HexSubstr(all_pmids[i].name().data.string())
              << (i < 2 ? " (bootstrap)" : "") << std::endl;
}

bool CreateKeys(const size_t &pmids_count, PmidVector &all_pmids) {
  all_pmids.clear();
  for (size_t i = 0; i < pmids_count; ++i) {
    try {
      maidsafe::passport::Anmaid anmaid;
      maidsafe::passport::Maid maid(anmaid);
      maidsafe::passport::Pmid pmid(maid);
      all_pmids.push_back(pmid);
    }
    catch(const std::exception& /*ex*/) {
      LOG(kError) << "CreatePmids - Could not create ID #" << i;
      return false;
    }
  }
  return true;
}

fs::path GetPathFromProgramOption(const std::string &option_name,
                                  po::variables_map *variables_map,
                                  bool is_dir,
                                  bool create_new_if_absent) {
  fs::path option_path;
  if (variables_map->count(option_name))
    option_path = variables_map->at(option_name).as<std::string>();
  if (option_path.empty())
    return fs::path();

  boost::system::error_code ec;
  if (!fs::exists(option_path, ec) || ec) {
    if (!create_new_if_absent) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " doesn't exist or can't be accessed (" << ec.message() << ")";
      return fs::path();
    }

    if (is_dir) {  // Create new dir
      fs::create_directories(option_path, ec);
      if (ec) {
        LOG(kError) << "GetPathFromProgramOption - Unable to create new dir " << option_path << " ("
                    << ec.message() << ")";
        return fs::path();
      }
    } else {  // Create new file
      if (option_path.has_filename()) {
        try {
          std::ofstream ofs(option_path.c_str());
        }
        catch(const std::exception &e) {
          LOG(kError) << "GetPathFromProgramOption - Exception while creating new file: "
                      << e.what();
          return fs::path();
        }
      }
    }
  }

  if (is_dir) {
    if (!fs::is_directory(option_path, ec) || ec) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " is not a directory (" << ec.message() << ")";
      return fs::path();
    }
  } else {
    if (!fs::is_regular_file(option_path, ec) || ec) {
      LOG(kError) << "GetPathFromProgramOption - Invalid " << option_name << ", " << option_path
                  << " is not a regular file (" << ec.message() << ")";
      return fs::path();
    }
  }

  LOG(kInfo) << "GetPathFromProgramOption - " << option_name << " is " << option_path;
  return option_path;
}

bool SetupNetwork(const PmidVector &all_pmids, bool bootstrap_only) {
  BOOST_ASSERT(all_pmids.size() >= 2);

  struct BootstrapData {
    BootstrapData()
        : routing1(),
          routing2(),
          info1(),
          info2() {}
    std::shared_ptr<maidsafe::routing::Routing> routing1, routing2;
    maidsafe::routing::NodeInfo info1, info2;
  } bootstrap_data;

  auto make_node_info = [&] (const maidsafe::passport::Pmid& pmid)->maidsafe::routing::NodeInfo {
    maidsafe::routing::NodeInfo node;
    node.node_id = maidsafe::NodeId(pmid.name().data.string());
    node.public_key = pmid.public_key();
    return node;
  };

  LOG(kInfo) << "Creating zero state routing network...";
  bootstrap_data.info1 = make_node_info(all_pmids[0]);
  bootstrap_data.routing1.reset(new maidsafe::routing::Routing(&(all_pmids[0])));
  bootstrap_data.info2 = make_node_info(all_pmids[1]);
  bootstrap_data.routing2.reset(new maidsafe::routing::Routing(&(all_pmids[1])));

  maidsafe::nfs::PublicKeyGetter public_key_getter(*bootstrap_data.routing1, all_pmids);

  maidsafe::routing::Functors functors1, functors2;
  functors1.request_public_key = functors2.request_public_key =
      [&public_key_getter](maidsafe::NodeId node_id,
                            const maidsafe::routing::GivePublicKeyFunctor& give_key) {
          public_key_getter.HandleGetKey(node_id, give_key);
      };

  boost::asio::ip::udp::endpoint endpoint1(maidsafe::GetLocalIp(),
                                           maidsafe::test::GetRandomPort());
  boost::asio::ip::udp::endpoint endpoint2(maidsafe::GetLocalIp(),
                                           maidsafe::test::GetRandomPort());
  auto a1 = std::async(std::launch::async, [&] {
    return bootstrap_data.routing1->ZeroStateJoin(functors1,
                                                  endpoint1,
                                                  endpoint2,
                                                  bootstrap_data.info2);
  });
  auto a2 = std::async(std::launch::async, [&] {
    return bootstrap_data.routing2->ZeroStateJoin(functors2,
                                                  endpoint2,
                                                  endpoint1,
                                                  bootstrap_data.info1);
  });
  if (a1.get() != 0 || a2.get() != 0) {
    LOG(kError) << "SetupNetwork - Could not start bootstrap nodes.";
    return false;
  }

  if (bootstrap_only) {
    // just wait till process receives termination signal
    std::cout << "Bootstrap nodes are running, press Ctrl+C to terminate." << std::endl
              << "Endpoints: " << endpoint1 << " and "
              << endpoint2 << std::endl;
    signal(SIGINT, CtrlCHandler);
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [] { return ctrlc_pressed; });  // NOLINT
    std::cout << "Shutting down bootstrap nodes." << std::endl;
    return true;
  }

//   maidsafe::priv::lifestuff_manager::ClientController client_controller(
//       [](const maidsafe::NonEmptyString&) {});
//   for (size_t i = 2; i < all_pmids.size(); ++i) {
//     if (client_controller.StartVault(all_pmids[i], all_pmids[i].name().first, "")) {
//       LOG(kSuccess) << "SetupNetwork - Started vault "
//                     << maidsafe::HexSubstr(all_pmids[i].name().first);
//     } else {
//       LOG(kError) << "SetupNetwork - Could not start vault "
//                   << maidsafe::HexSubstr(all_pmids[i].name().first);
//       return false;
//     }
//   }
//
//   maidsafe::Sleep(boost::posix_time::seconds(1));  // to keep bootstrap nodes alive for a bit

  return true;
}

bool StoreKeys(const PmidVector& all_pmids,
               const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
  maidsafe::passport::Anmaid client_anmaid;
  maidsafe::passport::Maid client_maid(client_anmaid);
  maidsafe::passport::Pmid client_pmid(client_maid);
  maidsafe::routing::Routing client_routing(nullptr);
  maidsafe::routing::Functors functors;

  std::promise<bool> join_promise;
  auto join_future = join_promise.get_future();
  functors.network_status = [&join_promise](int result) { join_promise.set_value(result == 0);};
  maidsafe::routing::Parameters::append_local_live_port_endpoint = true;  // To allow bootstrapping off vaults on local machine
  client_routing.Join(functors, peer_endpoints);

  if (!join_future.get()) {
    LOG(kError) << "Failed to bootstrap anonymous node for storing keys";
    return false;
  }
  LOG(kInfo) << "Bootstrapped anonymous node to store keys";

  maidsafe::nfs::ClientMaidNfs client_nfs(client_routing, client_maid);

  std::atomic<size_t> error_stored_keys(0);
  // on_error call back
  maidsafe::nfs::OnError cb = [&error_stored_keys](maidsafe::nfs::Message /*result*/) {
      ++error_stored_keys;
  };
  auto store_keys = [&client_nfs, &cb](const maidsafe::passport::Pmid& pmid,
                                       const maidsafe::passport::Pmid& /*owner*/) {
    maidsafe::passport::PublicPmid p_pmid(pmid);
    client_nfs.Put(p_pmid, cb);
  };
  maidsafe::AsioService asio_service(10);
  asio_service.Start();
  for (auto &pmid : all_pmids)  // store all PMIDs
    asio_service.service().post([&store_keys, pmid, all_pmids]() { store_keys(pmid, all_pmids[0]); });  // NOLINT

  // shall eventually use future
  maidsafe::Sleep(boost::posix_time::millisec(10000));

  asio_service.Stop();

  if (error_stored_keys > 0) {
    LOG(kError) << "StoreKeys - Could only store " << error_stored_keys << " out of "
                << all_pmids.size() << " keys.";
    return false;
  }
  LOG(kSuccess) << "StoreKeys - Stored all " << all_pmids.size() << " keys.";
  return true;
}

bool VerifyKeys(const PmidVector& all_pmids,
                const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
  maidsafe::passport::Anmaid client_anmaid;
  maidsafe::passport::Maid client_maid(client_anmaid);
  maidsafe::passport::Pmid client_pmid(client_maid);
  maidsafe::routing::Routing client_routing(&client_pmid);
  maidsafe::routing::Functors functors;

  std::promise<bool> join_promise;
  auto join_future = join_promise.get_future();
  functors.network_status = [&join_promise](int result) { join_promise.set_value(result == 0);};
  maidsafe::routing::Parameters::append_local_live_port_endpoint = true;  // To allow bootstrapping off vaults on local machine

  client_routing.Join(functors, peer_endpoints);

  if (!join_future.get()) {
    LOG(kError) << "Failed to bootstrap anonymous node to verify keys";
    return false;
  }

  LOG(kInfo) << "Bootstrapped anonymous node to verify keys";

  maidsafe::nfs::KeyGetterNfs key_getter_nfs(client_routing);

  std::atomic<size_t> verified_keys(0);
  auto verify_keys = [&key_getter_nfs, &verified_keys](const maidsafe::passport::Pmid& pmid) {
    maidsafe::passport::PublicPmid p_pmid(pmid);
    auto fetched_data = key_getter_nfs.Get<maidsafe::passport::PublicPmid>(p_pmid.name());
    maidsafe::passport::PublicPmid fetched_key = fetched_data.get();
    if (!fetched_key.name().data.IsInitialised()) {
      LOG(kError) << "VerifyKeys - Failed to retrieve "
                  << maidsafe::HexSubstr(pmid.name().data.string());
      return;
    }
    if (!maidsafe::asymm::MatchingKeys(fetched_key.public_key(), pmid.public_key())) {
      LOG(kError) << "VerifyKeys - fetched key mis-match "
                  << maidsafe::HexSubstr(pmid.name().data.string());
      return;
    }
    LOG(kSuccess) << "VerifyKeys - Successfully verified "
                  << maidsafe::HexSubstr(pmid.name().data.string());
    ++verified_keys;
  };

  std::vector<std::future<void>> futures;
  for (auto &pmid : all_pmids) {
    futures.push_back(std::async(std::launch::async, verify_keys, pmid));
  }
  for (auto &future : futures)
    future.get();

  if (verified_keys < all_pmids.size()) {
    LOG(kError) << "VerifyKeys - Could only verify " << verified_keys << " out of "
                << all_pmids.size() << " keys.";
    return false;
  }
  LOG(kSuccess) << "VerifyKeys - Verified all " << verified_keys << " keys.";
  return true;
}

// bool StoreChunks(const FobPairVector& all_keys,
//                  const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
//   BOOST_ASSERT(all_keys.size() >= 4);
//
//   std::unique_ptr<maidsafe::pd::Node> client(new maidsafe::pd::Node);
//   auto test_path = maidsafe::test::CreateTestPath("MaidSafe_Test_KeysHelper");
//   {
//     maidsafe::Fob fob(all_keys[2].first);  // MAID of ID #2
//     client->set_account_name(maidsafe::pd::AccountName(fob.identity.string()));
//     client->set_fob(fob);
//     if (client->Start(*test_path, peer_endpoints) != 0) {
//       std::cout << "Failed to start client to store chunks." << std::endl;
//       return false;
//     }
//   }
//   pcs::RemoteChunkStore rcs(client->chunk_store(), client->chunk_manager(),
//                             client->chunk_action_authority());
//
//   std::unique_ptr<maidsafe::pd::Node> client2(new maidsafe::pd::Node);
//   auto test_path2 = maidsafe::test::CreateTestPath("MaidSafe_Test_KeysHelper");
//   {
//     maidsafe::Fob fob(all_keys[3].first);  // MAID of ID #3
//     client2->set_account_name(fob.identity);
//     client2->set_fob(fob);
//     if (client2->Start(*test_path2, peer_endpoints) != 0) {
//       std::cout << "Failed to start client to retrieve chunks." << std::endl;
//       return false;
//     }
//   }
//   pcs::RemoteChunkStore rcs2(client2->chunk_store(), client2->chunk_manager(),
//                              client2->chunk_action_authority());
//
//   std::cout << "Going to store chunks, press Ctrl+C to stop." << std::endl;
//   signal(SIGINT, CtrlCHandler);
//   size_t num_chunks(0), num_store(0), num_get(0);
//   std::vector<std::pair<maidsafe::pd::ChunkName, maidsafe::NonEmptyString>> chunks;
//   boost::mutex::scoped_lock lock(mutex_);
//   while(!cond_var_.timed_wait(lock, bptime::seconds(3), [] { return ctrlc_pressed; })) {  // NOLINT
//     maidsafe::NonEmptyString content(maidsafe::RandomString(1 << 18));  // 256 KB
//     maidsafe::pd::ChunkName name(
//         maidsafe::crypto::Hash<maidsafe::crypto::SHA512>(content).string());
//     bool success(false);
//     ++num_chunks;
//     std::cout << "Storing chunk " << maidsafe::pd::DebugChunkName(name) << " ..." << std::endl;
//     if (rcs.Store(name, content, [&success](bool result) { success = result; }, maidsafe::Fob()) &&
//         rcs.WaitForCompletion() &&
//         success) {
//       std::cout << "Stored chunk " << maidsafe::pd::DebugChunkName(name) << std::endl;
//       ++num_store;
//       chunks.push_back(std::make_pair(name, content));
//     } else {
//       std::cout << "Failed to store chunk " << maidsafe::pd::DebugChunkName(name) << std::endl;
//     }
//   }
//   std::cout << "Stored " << num_store << " out of " << num_chunks << " chunks." << std::endl;
//
//   ctrlc_pressed = false;  // reset
//
//   std::cout << "Going to retrieve " << chunks.size() << " chunks, press Ctrl+C to stop."
//             << std::endl;
//   size_t i = 0;
//   while (!cond_var_.timed_wait(lock, boost::posix_time::millisec(100),
//                                [&i, &chunks] { return i >= chunks.size() || ctrlc_pressed; })) {  // NOLINT
//     std::cout << "Retrieving chunk " << maidsafe::pd::DebugChunkName(chunks[i].first) << " ..."
//               << std::endl;
//     std::string content(rcs2.Get(chunks[i].first, maidsafe::Fob()));  // retrieve with second client
//     if (content == chunks[i].second.string()) {
//       std::cout << "Retrieved chunk " << maidsafe::pd::DebugChunkName(chunks[i].first) << std::endl;
//       ++num_get;
//     } else {
//       std::cout << "Failed to retrieve chunk " << maidsafe::pd::DebugChunkName(chunks[i].first)
//                 << std::endl;
//     }
//     ++i;
//   }
//   std::cout << "Stored " << num_store << " and retrieved " << num_get << " out of " << num_chunks
//             << " chunks." << std::endl;
//
//   client->Stop();
//   client2->Stop();
//   return num_get == num_chunks;
// }

// bool ExtendedTest(const size_t& chunk_set_count,
//                   const FobPairVector& all_keys,
//                   const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints) {
//   const size_t kNumClients(3);
//   BOOST_ASSERT(all_keys.size() >= 2 + kNumClients);
//   std::vector<std::unique_ptr<maidsafe::pd::Node>> clients;
//   std::vector<maidsafe::test::TestPath> client_paths;
//   std::vector<std::unique_ptr<pcs::RemoteChunkStore>> rcs;
//   for (size_t i = 2; i < 2 + kNumClients; ++i) {
//     std::cout << "Setting up client " << (i - 1) << " ..." << std::endl;
//     std::unique_ptr<maidsafe::pd::Node> client(new maidsafe::pd::Node);
//     client_paths.push_back(maidsafe::test::CreateTestPath("MaidSafe_Test_KeysHelper"));
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
//   typedef std::map<maidsafe::pd::ChunkName,
//                    std::pair<maidsafe::NonEmptyString, maidsafe::Fob>> ChunkMap;
//   ChunkMap chunks;
//   for (size_t i = 0; i < kNumChunks; ++i) {
//     maidsafe::pd::ChunkName chunk_name;
//     maidsafe::NonEmptyString chunk_contents;
//     auto chunk_fob = clients[0]->fob();
//     switch (i % 3) {
//       case 0:  // DEF
//         chunk_contents = maidsafe::NonEmptyString(maidsafe::RandomString(1 << 18));  // 256 KB
//         chunk_name = maidsafe::priv::ApplyTypeToName(
//             maidsafe::crypto::Hash<maidsafe::crypto::SHA512>(chunk_contents),
//             maidsafe::priv::ChunkType::kDefault);
//         break;
//       case 1:  // MBO
//         maidsafe::pd::CreateSignedDataPayload(
//             maidsafe::NonEmptyString(maidsafe::RandomString(1 << 16)),  // 64 KB
//             chunk_fob.keys.private_key,
//             chunk_contents);
//         chunk_name = maidsafe::priv::ApplyTypeToName(maidsafe::NodeId(maidsafe::NodeId::kRandomId),
//                                                      maidsafe::priv::ChunkType::kModifiableByOwner);
//         break;
//       case 2:  // SIG
//         chunk_fob = maidsafe::pd::GenerateIdentity();
//         maidsafe::pd::CreateSignaturePacket(chunk_fob, chunk_name, chunk_contents);
//         break;
//     }
//     chunks[chunk_name] = std::make_pair(chunk_contents, chunk_fob);
//     LOG(kInfo) << "ExtendedTest - Generated chunk "
//                << maidsafe::pd::DebugChunkName(chunk_name) << " of size "
//                << maidsafe::BytesToBinarySiUnits(chunk_contents.string().size());
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
//           maidsafe::NonEmptyString new_content;
//           maidsafe::pd::CreateSignedDataPayload(
//               maidsafe::NonEmptyString(maidsafe::RandomString(1 << 16)),  // 64 KB
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
//                     << maidsafe::pd::DebugChunkName(chunk.first) << " "
//                     << (result ? "succeeded" : "failed") << " as expected.";
//     } else {
//       LOG(kError) << "ExtendedTest - " << action_name << " "
//                   << maidsafe::pd::DebugChunkName(chunk.first) << " "
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
//       auto chunk_type = maidsafe::priv::GetChunkType(chunk.first);
//       switch (stage) {
//         case 0:  // store all chunks
//           do_rcs_op(RcsOp::kStore, chunk, true);
//           break;
//         case 1:  // get all chunks to verify storage
//           do_rcs_op(RcsOp::kGet, chunk, true);
//           break;
//         case 2:  // modify all chunks, should only succeed for MBO
//           do_rcs_op(RcsOp::kModify, chunk,
//                     chunk_type == maidsafe::priv::ChunkType::kModifiableByOwner);
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
//                     chunk_type != maidsafe::priv::ChunkType::kSignaturePacket);
//           break;
//         case 8:  // get all chunks again, only SIG should fail due to revokation
//           do_rcs_op(RcsOp::kGet, chunk, chunk_type != maidsafe::priv::ChunkType::kSignaturePacket);
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

}  // unnamed namespace

int main(int argc, char* argv[]) {
  maidsafe::log::Logging::Instance().Initialise(argc, argv);

  std::cout << kHelperVersion << std::endl;

  int result(0);
  boost::system::error_code error_code;

  size_t pmids_count(12), chunk_set_count(3);

#ifdef NDEBUG
  try {
#endif
    // Options allowed only on command line
    po::options_description generic_options("Commands");
    generic_options.add_options()
        ("help,h", "Print this help message.")
        ("create,c", "Create keys and write to file.")
        ("load,l", "Load keys from file.")
        ("run,r", "Run vaults with available keys.")
        ("bootstrap,b", "Run boostrap nodes only, using first 2 keys.")
        ("store,s", "Store keys on network.")
        ("verify,v", "Verify keys are available on network.")
        ("test,t", "Run simple test that stores and retrieves chunks.")
        ("extended,x", "Run extended test that tries all operations on all chunk types.")
        ("delete,d", "Delete keys file.")
        ("print,p", "Print the list of keys available.");

    // Options allowed both on command line and in config file
    po::options_description config_file_options("Configuration options");
    config_file_options.add_options()
        ("peer",
            po::value<std::string>(),
            "Endpoint of bootstrap node, if attaching to running network.")
        ("pmids_count,n",
            po::value<size_t>(&pmids_count)->default_value(pmids_count),
            "Number of keys to create")
        ("keys_path",
            po::value<std::string>()->default_value(
                fs::path(fs::temp_directory_path(error_code) / "key_directory.dat").string()),
            "Path to keys file")
        ("chunk_path",
            po::value<std::string>()->default_value(
                fs::path(fs::temp_directory_path(error_code) / "keys_chunks").string()),
            "Path to chunk directory")
        ("chunk_set_count",
            po::value<size_t>(&chunk_set_count)->default_value(chunk_set_count),
            "Number of chunk sets to run extended test on");

    po::options_description cmdline_options;
    cmdline_options.add(generic_options).add(config_file_options);

    po::variables_map variables_map;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).allow_unregistered().
                                                  allow_unregistered().run(), variables_map);
    po::notify(variables_map);

    bool do_create(variables_map.count("create") != 0);
    bool do_load(variables_map.count("load") != 0);
    bool do_run(variables_map.count("run") != 0);
    bool do_bootstrap(variables_map.count("bootstrap") != 0);
    bool do_store(variables_map.count("store") != 0);
    bool do_verify(variables_map.count("verify") != 0);
    bool do_test(variables_map.count("test") != 0);
    bool do_extended(variables_map.count("extended") != 0);
    bool do_delete(variables_map.count("delete") != 0);
    bool do_print(variables_map.count("print") != 0);

    if (variables_map.count("help") ||
        (!do_create && !do_load && !do_run && !do_bootstrap && !do_store && !do_verify &&
         !do_test && !do_extended && !do_delete && !do_print)) {
      std::cout << cmdline_options << std::endl
                << "Commands are executed in this order: [c|l] p [r|b] s v t x d" << std::endl;
      return 0;
    }

    PmidVector all_pmids;
    fs::path keys_path(GetPathFromProgramOption("keys_path", &variables_map, false, true));
    fs::path chunk_path(GetPathFromProgramOption("chunk_path", &variables_map, true, true));

    // bootstrap endpoint
    std::vector<boost::asio::ip::udp::endpoint> peer_endpoints;
    if (variables_map.count("peer")) {
      std::string peer = variables_map.at("peer").as<std::string>();
      size_t delim = peer.rfind(':');
      try {
        boost::asio::ip::udp::endpoint ep;
        ep.port(boost::lexical_cast<uint16_t>(peer.substr(delim + 1)));
        ep.address(boost::asio::ip::address::from_string(peer.substr(0, delim)));
        peer_endpoints.push_back(ep);
        LOG(kInfo) << "Going to bootstrap off endpoint " << ep;
      }
      catch(...) {
        std::cout << "Could not parse IPv4 peer endpoint from " << peer << std::endl;
      }
    }

    if (do_create) {
      if (CreateKeys(pmids_count, all_pmids)) {
        std::cout << "Created " << all_pmids.size() << " pmids." << std::endl;
        if (maidsafe::passport::detail::WritePmidList(keys_path, all_pmids))
          std::cout << "Wrote keys to " << keys_path << std::endl;
        else
          std::cout << "Could not write keys to " << keys_path << std::endl;
      } else {
        std::cout << "Could not create keys." << std::endl;
      }
    } else if (do_load) {
      try {
        all_pmids = maidsafe::passport::detail::ReadPmidList(keys_path);
        std::cout << "Loaded " << all_pmids.size() << " pmids from " << keys_path << std::endl;
      }
      catch(const std::exception& /*ex*/) {
        all_pmids.clear();
        std::cout << "Could not load keys from " << keys_path << std::endl;
      }
    }

    if (do_print)
      PrintKeys(all_pmids);

    if (do_run || do_bootstrap) {
      if (all_pmids.size() < 3) {
        std::cout << "Need to have at least 3 keys to setup network." << std::endl;
        result = -1;
      } else if (!SetupNetwork(all_pmids, !do_run && do_bootstrap)) {
        std::cout << "Could not setup network." << std::endl;
        result = -1;
      } else if (do_run) {
        std::cout << "Vaults are running." << std::endl;
      }
    }

    if (do_store) {
      if (StoreKeys(all_pmids, peer_endpoints)) {
        std::cout << "Stored keys on network." << std::endl;
      } else {
        std::cout << "Could not store all keys on network." << std::endl;
        result = -1;
      }
    }

    if (do_verify) {
      if (VerifyKeys(all_pmids, peer_endpoints)) {
        std::cout << "Verified keys on network." << std::endl;
      } else {
        std::cout << "Could not verify all keys on network." << std::endl;
        result = -1;
      }
    }

//     if (do_test) {
//       if (all_pmids.size() < 4) {
//         std::cout << "Need to have at least 4 keys to run test." << std::endl;
//         result = -1;
//       } else if (!StoreChunks(all_pmids, peer_endpoints)) {
//         std::cout << "Could not store and retrieve test chunks." << std::endl;
//         result = -1;
//       }
//     }

//     if (do_extended) {
//       if (all_pmids.size() < 5) {
//         std::cout << "Need to have at least 5 keys to run extended test." << std::endl;
//         result = -1;
//       } else if (!ExtendedTest(chunk_set_count, all_pmids, peer_endpoints)) {
//         std::cout << "Extended test failed." << std::endl;
//         result = -1;
//       }
//     }

    if (do_delete) {
      if (fs::remove(keys_path, error_code))
        std::cout << "Deleted " << keys_path << std::endl;
      else
        std::cout << "Could not delete " << keys_path << std::endl;
    }
#ifdef NDEBUG
  }
  catch(const std::exception& exception) {
    std::cout << "Error: " << exception.what() << std::endl;
    result = -2;
  }
#endif

  return result;
}
