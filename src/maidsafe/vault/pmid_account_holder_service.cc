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

#include "maidsafe/vault/pmid_account_holder_service.h"

#include "maidsafe/common/error.h"

namespace maidsafe {

namespace vault {

PmidAccountHolder::PmidAccountHolder(routing::Routing& routing,
                                     nfs::PublicKeyGetter& public_key_getter,
                                     const boost::filesystem::path vault_root_dir)
  : routing_(routing),
    public_key_getter_(public_key_getter),
    accumulator_(),
    pmid_account_handler_(vault_root_dir),
    nfs_(routing) {}


void PmidAccountHolder::CloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {  //  NOLINT (fine when not commented)
}

void PmidAccountHolder::ValidateDataMessage(const nfs::DataMessage& data_message) {
  if (!data_message.HasTargetId()) {
    LOG(kError) << "No target ID, can't forward the message.";
    ThrowError(VaultErrors::permission_denied);
  }

  if (routing_.EstimateInGroup(data_message.this_persona().node_id,
                               NodeId(data_message.data().name))) {
    LOG(kError) << "Message doesn't seem to come from the right group.";
    ThrowError(VaultErrors::permission_denied);
  }
}

}  // namespace vault

}  // namespace maidsafe
