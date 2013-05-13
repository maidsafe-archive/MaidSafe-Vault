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

#include "maidsafe/routing/parameters.h"
#include "maidsafe/nfs/utils.h"


namespace maidsafe {

namespace vault {

namespace {

inline bool SenderInGroupForClientMaid(const nfs::Message& message, routing::Routing& routing) {
  return routing.EstimateInGroup(message.source().node_id,
                                 NodeId(message.client_validation().name.string()));
}

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kMetadataManager;
}

}  // unnamed namespace


const int MetadataManagerService::kPutRequestsRequired_(3);
const int MetadataManagerService::kPutRepliesSuccessesRequired_(3);
const int MetadataManagerService::kDeleteRequestsRequired_(3);


MetadataManagerService::MetadataManagerService(const passport::Pmid& pmid,
                                               routing::Routing& routing,
                                               nfs::PublicKeyGetter& public_key_getter,
                                               const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_mutex_(),
      accumulator_(),
      metadata_handler_(vault_root_dir),
      nfs_(routing, pmid) {}

void MetadataManagerService::HandleChurnEvent(routing::MatrixChange /*matrix_change*/) {
}

void MetadataManagerService::ValidatePutSender(const nfs::Message& message) const {
  if (!SenderInGroupForClientMaid(message, routing_) || !ThisVaultInGroupForData(message)) {
    ThrowError(VaultErrors::permission_denied);
  }

  if (!FromMaidAccountHolder(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MetadataManagerService::ValidateGetSender(const nfs::Message& message) const {
  if (!(FromClientMaid(message) ||
          FromDataHolder(message) ||
          FromDataGetter(message) ||
          FromOwnerDirectoryManager(message) ||
          FromGroupDirectoryManager(message) ||
          FromWorldDirectoryManager(message)) ||
      !ForThisPersona(message)) {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void MetadataManagerService::ValidateDeleteSender(const nfs::Message& message) const {
  if (!SenderInGroupForClientMaid(message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromMaidAccountHolder(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void MetadataManagerService::ValidatePostSender(const nfs::Message& message) const {
  if (!(FromMetadataManager(message) || FromPmidAccountHolder(message)) ||
      !ForThisPersona(message)) {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

//void MetadataManagerService::SendSyncData() {}

void MetadataManagerService::HandleNodeDown(const nfs::Message& /*message*/) {
  try {
    int online_holders(-1);
//    metadata_handler_.MarkNodeDown(message.name(), PmidName(), online_holders);
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

void MetadataManagerService::HandleNodeUp(const nfs::Message& /*message*/) {
  //try {
  //  metadata_handler_.MarkNodeUp(message.name(),
  //                               PmidName(Identity(message.name().string())));
  //}
  //catch(const std::exception &e) {
  //  LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
  //  return;
  //}
}

bool MetadataManagerService::ThisVaultInGroupForData(const nfs::Message& message) const {
  return routing::GroupRangeStatus::kInRange ==
         routing_.IsNodeIdInGroupRange(NodeId(message.data().name.string()));
}


}  // namespace vault

}  // namespace maidsafe
