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


#include "maidsafe/vault/demultiplexer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/immutable_data.h"
#include "maidsafe/data_types/owner_directory.h"
#include "maidsafe/data_types/group_directory.h"
#include "maidsafe/data_types/world_directory.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/vault/data_holder_service.h"
#include "maidsafe/vault/maid_account_holder_service.h"
#include "maidsafe/vault/metadata_manager_service.h"
#include "maidsafe/vault/pmid_account_holder_service.h"


namespace maidsafe {

namespace vault {

namespace {

template<typename Persona>
void HandleDataType(nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor,
                    Persona& persona) {
  // static assert
  switch (data_message.data().type) {
    case DataTagValue::kAnmidValue:
      return persona.template HandleDataMessage<passport::PublicAnmid>(data_message, reply_functor);
    case DataTagValue::kAnsmidValue:
      return persona.template HandleDataMessage<passport::PublicAnsmid>(data_message,
                                                                        reply_functor);
    case DataTagValue::kAntmidValue:
      return persona.template HandleDataMessage<passport::PublicAntmid>(data_message,
                                                                        reply_functor);
    case DataTagValue::kAnmaidValue:
      return persona.template HandleDataMessage<passport::PublicAnmaid>(data_message,
                                                                        reply_functor);
    case DataTagValue::kMaidValue:
      return persona.template HandleDataMessage<passport::PublicMaid>(data_message, reply_functor);
    case DataTagValue::kPmidValue:
      return persona.template HandleDataMessage<passport::PublicPmid>(data_message, reply_functor);
    case DataTagValue::kMidValue:
      return persona.template HandleDataMessage<passport::Mid>(data_message, reply_functor);
    case DataTagValue::kSmidValue:
      return persona.template HandleDataMessage<passport::Smid>(data_message, reply_functor);
    case DataTagValue::kTmidValue:
      return persona.template HandleDataMessage<passport::Tmid>(data_message, reply_functor);
    case DataTagValue::kAnmpidValue:
      return persona.template HandleDataMessage<passport::PublicAnmpid>(data_message,
                                                                        reply_functor);
    case DataTagValue::kMpidValue:
      return persona.template HandleDataMessage<passport::PublicMpid>(data_message, reply_functor);
    case DataTagValue::kImmutableDataValue:
      return persona.template HandleDataMessage<ImmutableData>(data_message, reply_functor);
    case DataTagValue::kOwnerDirectoryValue:
      return persona.template HandleDataMessage<OwnerDirectory>(data_message, reply_functor);
    case DataTagValue::kGroupDirectoryValue:
      return persona.template HandleDataMessage<GroupDirectory>(data_message, reply_functor);
    case DataTagValue::kWorldDirectoryValue:
      return persona.template HandleDataMessage<WorldDirectory>(data_message, reply_functor);
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // unnamed namespace


Demultiplexer::Demultiplexer(MaidAccountHolder& maid_account_holder,
                             MetadataManagerService& metadata_manager_service,
                             PmidAccountHolder& pmid_account_holder,
                             DataHolder& data_holder)
    : maid_account_holder_(maid_account_holder),
      metadata_manager_service_(metadata_manager_service),
      pmid_account_holder_(pmid_account_holder),
      data_holder_(data_holder) {}

void Demultiplexer::HandleMessage(const std::string& serialised_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    switch (message.inner_message_type()) {
      case nfs::MessageCategory::kData: {
        nfs::DataMessage data_message(message.serialised_inner_message<nfs::DataMessage>());
        return HandleDataMessagePersona(data_message, reply_functor);
      }
      case nfs::MessageCategory::kGeneric: {
        nfs::GenericMessage generic_msg(message.serialised_inner_message<nfs::GenericMessage>());
        return HandleGenericMessagePersona(generic_msg, reply_functor);
      }
      default:
        LOG(kError) << "Unhandled inner_message_type";
    }
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message: " << ex.what();
  }
}

void Demultiplexer::HandleDataMessagePersona(nfs::DataMessage& data_message,
                                             const routing::ReplyFunctor& reply_functor) {
  switch (data_message.destination_persona()) {
    case nfs::Persona::kMaidAccountHolder:
      return HandleDataType<MaidAccountHolder>(data_message, reply_functor, maid_account_holder_);
    case nfs::Persona::kMetadataManager:
      return HandleDataType<MetadataManagerService>(data_message, reply_functor, metadata_manager_service_);
    case nfs::Persona::kPmidAccountHolder:
      return HandleDataType<PmidAccountHolder>(data_message, reply_functor, pmid_account_holder_);
    case nfs::Persona::kDataHolder:
      return HandleDataType<DataHolder>(data_message, reply_functor, data_holder_);
    default:
      LOG(kError) << "Unhandled Persona";
  }
}

void Demultiplexer::HandleGenericMessagePersona(nfs::GenericMessage& /*generic_message*/,
                                                const routing::ReplyFunctor& /*reply_functor*/) {
}

bool Demultiplexer::GetFromCache(std::string& serialised_message) {
  try {
    nfs::Message request_message(
        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::DataMessage request_data_message(
        (request_message.serialised_inner_message<nfs::DataMessage>()));
    auto cached_content(HandleGetFromCache(request_data_message));
    if (cached_content.IsInitialised()) {
      nfs::DataMessage response_data_message(
          request_data_message.data().action,
          request_data_message.destination_persona(),
          request_data_message.source(),
          nfs::DataMessage::Data(request_data_message.data().type,
                                 request_data_message.data().name,
                                 cached_content));
      nfs::Message response_message(nfs::DataMessage::message_type_identifier,
                                    response_data_message.Serialise().data);
      serialised_message = response_message.Serialise()->string();
      return true;
    }
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling get from cache request: " << ex.what();
  }
  return false;
}

NonEmptyString Demultiplexer::HandleGetFromCache(nfs::DataMessage& data_message) {
  switch (data_message.data().type) {
    case DataTagValue::kAnmidValue:
      return data_holder_.GetFromCache<passport::PublicAnmid>(data_message);
    case DataTagValue::kAnsmidValue:
      return data_holder_.GetFromCache<passport::PublicAnsmid>(data_message);
    case DataTagValue::kAntmidValue:
      return data_holder_.GetFromCache<passport::PublicAntmid>(data_message);
    case DataTagValue::kAnmaidValue:
      return data_holder_.GetFromCache<passport::PublicAnmaid>(data_message);
    case DataTagValue::kMaidValue:
      return data_holder_.GetFromCache<passport::PublicMaid>(data_message);
    case DataTagValue::kPmidValue:
      return data_holder_.GetFromCache<passport::PublicPmid>(data_message);
    case DataTagValue::kAnmpidValue:
      return data_holder_.GetFromCache<passport::PublicAnmpid>(data_message);
    case DataTagValue::kMpidValue:
      return data_holder_.GetFromCache<passport::PublicMpid>(data_message);
    case DataTagValue::kImmutableDataValue:
      return data_holder_.GetFromCache<ImmutableData>(data_message);
    case DataTagValue::kOwnerDirectoryValue:
      return data_holder_.GetFromCache<OwnerDirectory>(data_message);
    case DataTagValue::kGroupDirectoryValue:
      return data_holder_.GetFromCache<GroupDirectory>(data_message);
    case DataTagValue::kWorldDirectoryValue:
      return data_holder_.GetFromCache<WorldDirectory>(data_message);
    default:
      LOG(kError) << "Unhandled data type";
  }
  return NonEmptyString();
}

void Demultiplexer::StoreInCache(const std::string& serialised_message) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::DataMessage data_message((message.serialised_inner_message<nfs::DataMessage>()));
    HandleStoreInCache(data_message);
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling store in cache request: " << ex.what();
  }
}

void Demultiplexer::HandleStoreInCache(const nfs::DataMessage& data_message) {
  switch (data_message.data().type) {
    case DataTagValue::kAnmidValue:
      return data_holder_.StoreInCache<passport::PublicAnmid>(data_message);
    case DataTagValue::kAnsmidValue:
      return data_holder_.StoreInCache<passport::PublicAnsmid>(data_message);
    case DataTagValue::kAntmidValue:
      return data_holder_.StoreInCache<passport::PublicAntmid>(data_message);
    case DataTagValue::kAnmaidValue:
      return data_holder_.StoreInCache<passport::PublicAnmaid>(data_message);
    case DataTagValue::kMaidValue:
      return data_holder_.StoreInCache<passport::PublicMaid>(data_message);
    case DataTagValue::kPmidValue:
      return data_holder_.StoreInCache<passport::PublicPmid>(data_message);
    case DataTagValue::kAnmpidValue:
      return data_holder_.StoreInCache<passport::PublicAnmpid>(data_message);
    case DataTagValue::kMpidValue:
      return data_holder_.StoreInCache<passport::PublicMpid>(data_message);
    case DataTagValue::kImmutableDataValue:
      return data_holder_.StoreInCache<ImmutableData>(data_message);
    case DataTagValue::kOwnerDirectoryValue:
      return data_holder_.StoreInCache<OwnerDirectory>(data_message);
    case DataTagValue::kGroupDirectoryValue:
      return data_holder_.StoreInCache<GroupDirectory>(data_message);
    case DataTagValue::kWorldDirectoryValue:
      return data_holder_.StoreInCache<WorldDirectory>(data_message);
    default :
      LOG(kError) << "Unhandled data type";
  }
}

}  // namespace vault

}  // namespace maidsafe
