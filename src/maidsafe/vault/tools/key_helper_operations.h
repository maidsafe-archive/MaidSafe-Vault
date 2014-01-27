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

#ifndef MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_
#define MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_

#include <atomic>
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

#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/node_info.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/nfs/client/maid_node_nfs.h"

namespace maidsafe {

namespace vault {

namespace tools {

typedef std::vector<passport::detail::AnmaidToPmid> KeyChainVector;
typedef std::vector<passport::Pmid> PmidVector;

class NetworkGenerator {
 public:
  NetworkGenerator();
  void SetupBootstrapNodes(const PmidVector& all_keys);
  std::vector<boost::asio::ip::udp::endpoint> BootstrapEndpoints() const;

 private:
  AsioService asio_service_;
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
      node.node_id = NodeId(pmid.name()->string());
      node.public_key = pmid.public_key();
      return node;
    }
  };

};

class ClientTester {
 public:
  ClientTester(const passport::detail::AnmaidToPmid& key_chain,
               const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints,
               const std::vector<passport::PublicPmid>& public_pmids_from_file,
               bool register_pmid_for_client);

 protected:
  // TODO(Dan): Remove the typedefs
  // I've put these because the Qt Creator parser is still missing >> recognition except when
  // it refers to the operator and that invalidates the parsing moving forward, which is annoying.
  typedef std::promise<bool> BoolPromise;
  typedef std::future<bool> BoolFuture;

  template <typename SigningData>
  bool EqualKeys(const SigningData& lhs, const SigningData& rhs) {
    return lhs.name() == rhs.name() && asymm::MatchingKeys(lhs.public_key(), rhs.public_key());
  }

  AsioService asio_service_;
  passport::detail::AnmaidToPmid key_chain_;
  routing::Routing client_routing_;
  routing::Functors functors_;
  std::unique_ptr<nfs_client::MaidNodeNfs> client_nfs_;

  ~ClientTester();
  std::future<bool> RoutingJoin(const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);

 private:
  std::vector<passport::PublicPmid> kAllPmids_;
  std::vector<std::future<void>> getting_keys_;
  std::atomic<bool> call_once_;
};

class KeyStorer : public ClientTester {
 public:
  KeyStorer(const passport::detail::AnmaidToPmid& key_chain,
            const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints,
            const std::vector<passport::PublicPmid>& public_pmids_from_file,
            const KeyChainVector& key_chain_list_in);
  void Store();

 private:
  template <typename Data>
  void StoreKey(const Data& key) {
    client_nfs_->Put(key);
  }
  KeyChainVector key_chain_list;
};

class KeyVerifier : public ClientTester {
 public:
  KeyVerifier(const passport::detail::AnmaidToPmid& key_chain,
              const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints,
              const std::vector<passport::PublicPmid>& public_pmids_from_file);
  void Verify();
};

class DataChunkStorer : public ClientTester {
 public:
  DataChunkStorer(const passport::detail::AnmaidToPmid& key_chain,
                  const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints,
                  const std::vector<passport::PublicPmid>& public_pmids_from_file,
                  bool load_chunk_from_file);

  void StopTest();
  void Test(int32_t quantity = -1);
  void TestWithDelete(int32_t quantity = -1);
  void TestStoreChunk(int chunk_index);
  void TestFetchChunk(int chunk_index);
  void TestDeleteChunk(int chunk_index);

 private:
  std::atomic<bool> run_;
  std::vector<ImmutableData> chunk_list_;

  bool Done(int32_t quantity, int32_t rounds) const;
  void OneChunkRun(size_t& num_chunks, size_t& num_store, size_t& num_get);
  void OneChunkRunWithDelete(size_t& num_chunks, size_t& num_store, size_t& num_get);
  void StoreOneChunk(const ImmutableData& chunk_data);
  bool GetOneChunk(const ImmutableData& chunk_data);
  bool DeleteOneChunk(const ImmutableData& chunk_data);
  void LoadChunksFromFiles();
};

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TOOLS_KEY_HELPER_OPERATIONS_H_
