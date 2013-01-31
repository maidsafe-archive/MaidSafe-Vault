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

#include "maidsafe/vault/utils.h"

#include <string>

#include "maidsafe/common/types.h"

#include "maidsafe/vault/parameters.h"


namespace maidsafe {

namespace vault {

namespace detail {

// bool NodeRangeCheck(maidsafe::routing::Routing& routing, const NodeId& node_id) {
//   return routing.IsNodeIdInGroupRange(node_id);  // provisional call to Is..
// }

bool ShouldRetry(routing::Routing& routing, const nfs::DataMessage& data_message) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(data_message.source().node_id,
                                 NodeId(data_message.data().name.string()));
}

MaidName GetSourceMaidName(const nfs::DataMessage& data_message) {
  if (data_message.this_persona().persona != nfs::Persona::kClientMaid)
    ThrowError(VaultErrors::permission_denied);
  return MaidName(Identity(data_message.this_persona().node_id.string()));
}

std::vector<std::future<nfs::ReturnCode>> GetMappedNfsFutures(
    std::vector<std::future<std::string>>&& routing_futures,
    nfs::ResponseMapper& response_mapper) {
  std::vector<std::future<nfs::ReturnCode>> nfs_futures;
  for (auto& routing_future : routing_futures) {
    std::promise<nfs::ReturnCode> nfs_promise;
    std::future<nfs::ReturnCode> nfs_future(nfs_promise.get_future());
    response_mapper_.push_back(std::make_pair(std::move(routing_future),
                                                        std::move(nfs_promise)));
    nfs_futures.push_back(std::move(nfs_future));
  }
  return nfs_futures;
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
