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

#include "maidsafe/vault/metadata_manager/metadata_manager_service.h"

#include <string>
#include <vector>


namespace maidsafe {

namespace vault {

MetadataManagerService::MetadataManagerService(const passport::Pmid& pmid,
                                               routing::Routing& routing,
                                               nfs::PublicKeyGetter& public_key_getter,
                                               const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_(),
      metadata_handler_(vault_root_dir),
      nfs_(routing, pmid) {}

//MetadataManagerService::~MetadataManagerService() {}

void MetadataManagerService::TriggerSync() {
}

//void MetadataManagerService::Serialise() {}

void MetadataManagerService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                                  const routing::ReplyFunctor& /*reply_functor*/) {
  // TODO(Team): Validate message
  nfs::GenericMessage::Action action(generic_message.action());
  switch (action) {
    case nfs::GenericMessage::Action::kNodeUp:
      // No need to reply
      HandleNodeUp(generic_message);
      break;
    case nfs::GenericMessage::Action::kNodeDown:
      // No need to reply
      HandleNodeDown(generic_message);
      break;
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

//void MetadataManagerService::SendSyncData() {}

void MetadataManagerService::HandleNodeDown(const nfs::GenericMessage& /*generic_message*/) {
  try {
    int online_holders(-1);
//    metadata_handler_.MarkNodeDown(generic_message.name(), PmidName(), online_holders);
    if (online_holders < 3) {
      // TODO(Team): Get content. There is no manager available yet.

      // Select new holder
      NodeId new_holder(routing_.GetRandomExistingNode());

      // TODO(Team): Put content. There is no manager available yet.
    }
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeDown - Dropping process after exception: " << e.what();
    return;
  }
}

void MetadataManagerService::HandleNodeUp(const nfs::GenericMessage& /*generic_message*/) {
//  try {
//    metadata_handler_.MarkNodeUp(generic_message.name(), PmidName());
//  }
//  catch(const std::exception &e) {
//    LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
//    return;
//  }
}

}  // namespace vault

}  // namespace maidsafe
