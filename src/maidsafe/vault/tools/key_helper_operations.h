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

#ifndef MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_
#define MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_

#include <condition_variable>
#include <mutex>
#include <vector>
#include <string>

#include "boost/asio/ip/udp.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/program_options/variables_map.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/data_types/immutable_data.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/client_utils.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/node_info.h"

namespace maidsafe {

namespace vault {

namespace tools {

typedef std::vector<passport::detail::AnmaidToPmid> KeyChainVector;
typedef std::vector<passport::Pmid> PmidVector;

class NetworkGenerator {
 public:
  NetworkGenerator();
  void SetupBootstrapNodes(const PmidVector &all_keys);
  std::vector<boost::asio::ip::udp::endpoint> BootstrapEndpoints() const;

 private:
  boost::asio::ip::udp::endpoint endpoint1_, endpoint2_;
  struct BootstrapData {
    BootstrapData(const passport::Pmid& pmid1, const passport::Pmid& pmid2)
        : routing1(new routing::Routing(pmid1)),
          routing2(new routing::Routing(pmid2)),
          info1(MakeNodeInfo(pmid1)),
          info2(MakeNodeInfo(pmid2)) {}
    std::unique_ptr<routing::Routing> routing1, routing2;
    routing::NodeInfo info1, info2;

    routing::NodeInfo MakeNodeInfo(const passport::Pmid& pmid) {
      routing::NodeInfo node;
      node.node_id = NodeId(pmid.name().data.string());
      node.public_key = pmid.public_key();
      return node;
    }
  };

  void DoOnPublicKeyRequested(const NodeId& node_id,
                              const routing::GivePublicKeyFunctor& give_key,
                              nfs::PublicKeyGetter& public_key_getter);
};

class ClientTester {
 public:
  ClientTester(const passport::detail::AnmaidToPmid& key_chain,
               const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);

 protected:
  // TODO(Dan): Remove the typedefs
  // I've put these because the Qt Creator parser is still missing >> recognition except when
  // it refers to the operator and that invalidates the parsing moving forward, which is annoying.
  typedef std::promise<bool> BoolPromise;
  typedef std::future<bool> BoolFuture;

  passport::detail::AnmaidToPmid key_chain_;
  routing::Routing client_routing_;
  routing::Functors functors_;
  std::unique_ptr<nfs::ClientMaidNfs> client_nfs_;

  ~ClientTester();
  std::future<bool> RoutingJoin(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
};

class KeyStorer : public ClientTester {
 public:
  KeyStorer(const passport::detail::AnmaidToPmid& key_chain,
            const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
  void Store();

 private:
  template<typename Data>
  void StoreKey(const Data& key, std::promise<bool>& promise) {
    nfs::Put<Data>(*client_nfs_,
                   key,
                   passport::PublicPmid::name_type(),
                   routing::Parameters::node_group_size,
                   callback(std::ref(promise)));
  }

  std::function<void(nfs::Reply)> callback(std::promise<bool>& promise);
};

class KeyVerifier : public ClientTester {
 public:
  KeyVerifier(const passport::detail::AnmaidToPmid& key_chain,
              const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
  void Verify();

 private:
  template<typename SigningData>
  bool EqualKeys(const SigningData& lhs, const SigningData& rhs) {
    return lhs.name() == rhs.name() && asymm::MatchingKeys(lhs.public_key(), rhs.public_key());
  }
};

class DataChunkStorer : public ClientTester {
 public:
  DataChunkStorer(const passport::detail::AnmaidToPmid& key_chain,
                  const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);

  void StopTest();
  void Test(int32_t quantity = -1);
  void TestWithDelete(int32_t quantity = -1);

 private:
  std::atomic_bool run_;
  std::vector<ImmutableData> chunk_list_;

  bool Done(int32_t quantity, int32_t rounds) const;
  void OneChunkRun(size_t& num_chunks, size_t& num_store, size_t& num_get);
  void OneChunkRunWithDelete(size_t& num_chunks, size_t& num_store, size_t& num_get);
  bool StoreOneChunk(const ImmutableData& chunk_data);
  bool GetOneChunk(const ImmutableData& chunk_data);
  bool DeleteOneChunk(const ImmutableData& chunk_data);
  void LoadChunksFromFiles();
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_
