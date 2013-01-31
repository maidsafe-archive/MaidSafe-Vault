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

#include "maidsafe/vault/metadata_manager/metadata_manager_service_service.h"

#include <string>
#include <vector>


namespace maidsafe {

namespace vault {

MetadataManagerService::MetadataManagerService(routing::Routing& routing,
                                 const boost::filesystem::path& vault_root_dir)
    : kRootDir_(vault_root_dir),
      routing_(routing),
      data_elements_manager_(vault_root_dir),
      nfs_(routing),
      request_accumulator_() {}

MetadataManagerService::~MetadataManagerService() {}

void MetadataManagerService::CloseNodeReplaced(
    const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
}

void MetadataManagerService::Serialise() {}

void MetadataManagerService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                        const routing::ReplyFunctor& reply_functor) {
  // TODO(Team): Validate message
  nfs::GenericMessage::Action action(generic_message.action());
  NodeId source_id(generic_message.source().node_id);
  switch (action) {
    case nfs::GenericMessage::Action::kNodeUp:
        if (!HandleNodeUp(generic_message, source_id)) {
          LOG(kError) << "Replying with failure on kNodeUp.";
          reply_functor(nfs::ReturnCode(VaultErrors::failed_handle_node_up).Serialise()->string());
        }
        break;
    case nfs::GenericMessage::Action::kNodeDown:
        if (!HandleNodeDown(generic_message, source_id)) {
          LOG(kError) << "Replying with failure on kNodeDown.";
          reply_functor(nfs::ReturnCode(
                            VaultErrors::failed_handle_node_down).Serialise()->string());
        }
        break;
    default: LOG(kError) << "Unhandled Post action type";
  }
}

void MetadataManagerService::SendSyncData() {}

bool MetadataManagerService::HandleNodeDown(
    const nfs::GenericMessage& generic_message, NodeId& /*node*/) {
  try {
    int64_t online_holders(-1);
    data_elements_manager_.MoveNodeToOffline(generic_message.name(), PmidName(), online_holders);
    if (online_holders < 3) {
      // TODO(Team): Get content. There is no manager available yet.

      // Select new holder
      NodeId new_holder(routing_.GetRandomExistingNode());

      // TODO(Team): Put content. There is no manager available yet.
    }
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeDown - Dropping process after exception: " << e.what();
    return false;
  }

  return true;
}

bool MetadataManagerService::HandleNodeUp(
    const nfs::GenericMessage& generic_message, NodeId& /*node*/) {
  try {
    data_elements_manager_.MoveNodeToOnline(generic_message.name(), PmidName());
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
    return false;
  }

  return true;
}

}  // namespace vault

}  // namespace maidsafe
