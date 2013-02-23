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

#include <condition_variable>
#include <mutex>
#include <vector>

#include "boost/asio/ip/udp.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/program_options/variables_map.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/routing/api_config.h"


namespace maidsafe {

namespace vault {

namespace tools {

typedef std::vector<passport::Pmid> PmidVector;

const std::string kHelperVersion = "MaidSafe Vault KeysHelper " + kApplicationVersion;

void CtrlCHandler(int signum);
void PrintKeys(const PmidVector& all_pmids);
void CreateKeys(const size_t& pmids_count, PmidVector& all_pmids);
boost::filesystem::path GetPathFromProgramOption(
    const std::string& option_name,
    boost::program_options::variables_map& variables_map,
    bool is_dir,
    bool create_new_if_absent);
void DoOnPublicKeyRequested(const NodeId& node_id,
                            const routing::GivePublicKeyFunctor& give_key,
                            nfs::PublicKeyGetter& public_key_getter);
void SetupNetwork(const PmidVector& all_pmids, bool bootstrap_only);
std::future<bool> RoutingJoin(routing::Routing& routing,
                              routing::Functors& functors,
                              const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
bool StoreKeys(const PmidVector& all_pmids,
               const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
bool VerifyKeys(const PmidVector& all_pmids,
                const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);
bool StoreChunks(const PmidVector& all_pmids,
                 const std::vector<boost::asio::ip::udp::endpoint>& peer_endpoints);

}  // namespace tools

}  // namespace vault

}  // namespace maidsafe
