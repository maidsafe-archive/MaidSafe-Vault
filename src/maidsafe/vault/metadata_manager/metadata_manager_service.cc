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

bool MetadataManagerService::ValidateGetSender(const nfs::DataMessage& data_message) const {
    return ((data_message.source().persona == nfs::Persona::kClientMaid ||
             data_message.source().persona == nfs::Persona::kDataHolder ) &&
      data_message.destination_persona() == nfs::Persona::kMetadataManager);
}

bool MetadataManagerService::ValidateMAHSender(const nfs::DataMessage& data_message) const {
    return (data_message.source().persona == nfs::Persona::kMaidAccountHolder &&
      data_message.destination_persona() == nfs::Persona::kMetadataManager &&
            routing_.EstimateInGroup(NodeId(data_message.client_validation().name.string()),
                                     data_message.source().node_id));
}

bool MetadataManagerService::ValidateSender(const nfs::GenericMessage& generic_message) const {
    return ((generic_message.source().persona == nfs::Persona::kPmidAccountHolder ||
              generic_message.source().persona == nfs::Persona::kMetadataManager) &&
      generic_message.destination_persona() == nfs::Persona::kMetadataManager);
}


void MetadataManagerService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                                  const routing::ReplyFunctor& /*reply_functor*/) {
  if (!ValidatePAHSender(generic_message))
    return;  // silently drop

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

void MetadataManagerService::HandleNodeDown(const nfs::GenericMessage& generic_message) {
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

void MetadataManagerService::HandleNodeUp(const nfs::GenericMessage& generic_message) {
  try {
    metadata_handler_.MarkNodeUp(generic_message.name(),
                                 PmidName(generic_message.name().string()));
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
    return;
  }
}

void MetadataManagerService::SendReply(const nfs::DataMessage& original_message,
                                       const maidsafe_error& return_code,
                                       const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  if (return_code.code() != CommonErrors::success)
    reply = nfs::Reply(return_code, original_message.Serialise().data);
  accumulator_.SetHandled(original_message, reply.error());
  reply_functor(reply.Serialise()->string());
}

void MetadataManagerService::AddResult(const nfs::DataMessage& data_message,
               const routing::ReplyFunctor& reply_functor,
               const maidsafe_error& return_code) {
  auto accumulate(accumulator_.PushSingleResult(data_message, reply_functor,return_code));
  if (accumulate.size() >= 3) {
    SendReply(data_message, return_code, reply_functor);
    StoreData(data_message, reply_functor);
    accumulator_.SetHandled(data_message, return_code);
  }
}

}  // namespace vault

}  // namespace maidsafe
