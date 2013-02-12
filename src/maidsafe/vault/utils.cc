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

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
