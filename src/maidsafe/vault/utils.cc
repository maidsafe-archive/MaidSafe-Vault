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

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/parameters.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

void InitialiseDirectory(const boost::filesystem::path& directory) {
  if (fs::exists(directory)) {
    if (!fs::is_directory(directory))
      ThrowError(CommonErrors::not_a_directory);
  } else {
    fs::create_directory(directory);
  }
}

bool ShouldRetry(routing::Routing& routing, const nfs::DataMessage& data_message) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(data_message.source().node_id,
                                 NodeId(data_message.data().name.string()));
}

MaidName GetSourceMaidName(const nfs::DataMessage& data_message) {
  if (data_message.source().persona != nfs::Persona::kClientMaid)
    ThrowError(VaultErrors::permission_denied);
  return MaidName(Identity(data_message.source().node_id.string()));
}

void SendReply(const nfs::DataMessage& original_message,
               const maidsafe_error& return_code,
               const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  if (return_code.code() != CommonErrors::success)
    reply = nfs::Reply(return_code, original_message.Serialise().data);
  reply_functor(reply.Serialise()->string());
}

CheckHoldersResult CheckHolders(const routing::MatrixChange& matrix_change,
                                const NodeId& this_id,
                                const NodeId& target) {
  CheckHoldersResult holders_result;
  std::vector<NodeId> old_matrix(matrix_change.old_matrix),
      new_matrix(matrix_change.new_matrix);
  std::sort(old_matrix.begin(),
            old_matrix.end(),
            [target](const NodeId& lhs, const NodeId& rhs) {
              return NodeId::CloserToTarget(lhs, rhs, target);
            });
  std::sort(new_matrix.begin(),
            new_matrix.end(),
            [target](const NodeId& lhs, const NodeId& rhs) {
              return NodeId::CloserToTarget(lhs, rhs, target);
            });
  std::vector<NodeId> difference;
  auto itr(std::set_difference(new_matrix.begin(),
                               new_matrix.end(),
                               old_matrix.begin(),
                               old_matrix.end(),
                               difference.begin()));
  difference.resize(itr - difference.begin());
  holders_result.new_holders.insert(holders_result.new_holders.begin(),
                                    difference.begin(),
                                    difference.end());
  holders_result.this_node_status = routing::GroupRangeStatus::kOutwithRange;
  if (new_matrix.size() <= routing::Parameters::node_group_size ||
      !NodeId::CloserToTarget(new_matrix.at(routing::Parameters::node_group_size - 1),
                              this_id,
                              target))
    holders_result.this_node_status = routing::GroupRangeStatus::kInRange;
  else if (new_matrix.size() <= routing::Parameters::closest_nodes_size ||
      !NodeId::CloserToTarget(new_matrix.at(routing::Parameters::closest_nodes_size - 1),
                              this_id,
                              target))
   holders_result.this_node_status = routing::GroupRangeStatus::kInProximalRange;
  return holders_result;
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
