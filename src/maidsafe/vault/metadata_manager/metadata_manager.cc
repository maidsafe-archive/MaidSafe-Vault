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

#include "maidsafe/vault/metadata_manager/metadata_manager.h"

#include <string>
#include <vector>


namespace maidsafe {

namespace vault {

MetadataManager::MetadataManager(routing::Routing& routing,
                                 const boost::filesystem::path& vault_root_dir)
    : kRootDir_(vault_root_dir),
      routing_(routing),
      data_elements_manager_(vault_root_dir),
      nfs_(routing)/*,
      request_queue_()*/ {}

MetadataManager::~MetadataManager() {}

void MetadataManager::CloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
}

void MetadataManager::Serialise() {}

void MetadataManager::HandlePostMessage(const nfs::PostMessage& message,
                                        const routing::ReplyFunctor& reply_functor) {
  // TODO(Team): Validate message
  nfs::PostActionType action_type(message.post_action_type());
  NodeId source_id(message.source().node_id);
  switch (action_type) {
    case nfs::PostActionType::kConnect:
        if (!HandleNodeUp(message, source_id)) {
          LOG(kError) << "Replying with failure on kNodeUp.";
          reply_functor(nfs::ReturnCode(-1).Serialise()->string());
        }
        break;
    case nfs::PostActionType::kNodeDown:
        if (!HandleNodeDown(message, source_id)) {
          LOG(kError) << "Replying with failure on kNodeDown.";
          reply_functor(nfs::ReturnCode(-1).Serialise()->string());
        }
        break;
    default: LOG(kError) << "Unhandled Post action type";
  }
}

void MetadataManager::SendSyncData() {}

bool MetadataManager::HandleNodeDown(const nfs::PostMessage& message, NodeId& /*node*/) {
  // TODO(Team): Parse message. Can't be done until the post messages are available.
  //             The node id inside is the one that should be passed to the MoveNodeToOnline

  try {
    int64_t online_holders(-1);
    data_elements_manager_.MoveNodeToOffline(message.name(), PmidName(), online_holders);
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

bool MetadataManager::HandleNodeUp(const nfs::PostMessage& message, NodeId& /*node*/) {
  // TODO(Team): Parse message. Can't be done until the post messages are available.
  //             The node id inside is the one that should be passed to the MoveNodeToOnline
  try {
    data_elements_manager_.MoveNodeToOnline(message.name(), PmidName());
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
    return false;
  }

  return true;
}

}  // namespace vault

}  // namespace maidsafe
