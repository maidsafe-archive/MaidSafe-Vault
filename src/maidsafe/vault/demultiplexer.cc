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
#include "maidsafe/detail/data_type_values.h"
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
    case maidsafe::detail::DataTagValue::kAnmidValue:
      return persona.template HandleDataMessage<passport::PublicAnmid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kAnsmidValue:
      return persona.template HandleDataMessage<passport::PublicAnsmid>(data_message,
                                                                        reply_functor);
    case maidsafe::detail::DataTagValue::kAntmidValue:
      return persona.template HandleDataMessage<passport::PublicAntmid>(data_message,
                                                                        reply_functor);
    case maidsafe::detail::DataTagValue::kAnmaidValue:
      return persona.template HandleDataMessage<passport::PublicAnmaid>(data_message,
                                                                        reply_functor);
    case maidsafe::detail::DataTagValue::kMaidValue:
      return persona.template HandleDataMessage<passport::PublicMaid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kPmidValue:
      return persona.template HandleDataMessage<passport::PublicPmid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kMidValue:
      return persona.template HandleDataMessage<passport::Mid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kSmidValue:
      return persona.template HandleDataMessage<passport::Smid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kTmidValue:
      return persona.template HandleDataMessage<passport::Tmid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kAnmpidValue:
      return persona.template HandleDataMessage<passport::PublicAnmpid>(data_message,
                                                                        reply_functor);
    case maidsafe::detail::DataTagValue::kMpidValue:
      return persona.template HandleDataMessage<passport::PublicMpid>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kImmutableDataValue:
      return persona.template HandleDataMessage<ImmutableData>(data_message, reply_functor);
    case maidsafe::detail::DataTagValue::kMutableDataValue:
      return persona.template HandleDataMessage<MutableData>(data_message, reply_functor);
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // unnamed namespace

DataNameVariant GetDataNameVariant(int32_t type, const Identity& name) {
  switch (type) {
    case maidsafe::detail::DataTagValue::kAnmidValue:
      return DataNameVariant(passport::PublicAnmid::name_type(name));
    case maidsafe::detail::DataTagValue::kAnsmidValue:
      return DataNameVariant(passport::PublicAnsmid::name_type(name));
    case maidsafe::detail::DataTagValue::kAntmidValue:
      return DataNameVariant(passport::PublicAntmid::name_type(name));
    case maidsafe::detail::DataTagValue::kAnmaidValue:
      return DataNameVariant(passport::PublicAnmaid::name_type(name));
    case maidsafe::detail::DataTagValue::kMaidValue:
      return DataNameVariant(passport::PublicMaid::name_type(name));
    case maidsafe::detail::DataTagValue::kPmidValue:
      return DataNameVariant(passport::PublicPmid::name_type(name));
    case maidsafe::detail::DataTagValue::kMidValue:
      return DataNameVariant(passport::Mid::name_type(name));
    case maidsafe::detail::DataTagValue::kSmidValue:
      return DataNameVariant(passport::Smid::name_type(name));
    case maidsafe::detail::DataTagValue::kTmidValue:
      return DataNameVariant(passport::Tmid::name_type(name));
    case maidsafe::detail::DataTagValue::kAnmpidValue:
      return DataNameVariant(passport::PublicAnmpid::name_type(name));
    case maidsafe::detail::DataTagValue::kMpidValue:
      return DataNameVariant(passport::PublicMpid::name_type(name));
    case maidsafe::detail::DataTagValue::kImmutableDataValue:
      return DataNameVariant(ImmutableData::name_type(name));
    case maidsafe::detail::DataTagValue::kMutableDataValue:
      return DataNameVariant(MutableData::name_type(name));
    default:
      LOG(kError) << "Unhandled data type";
  }
}

Demultiplexer::Demultiplexer(MaidAccountHolder& maid_account_holder,
                             MetadataManager& metadata_manager,
                             PmidAccountHolder& pmid_account_holder,
                             DataHolder& data_holder)
    : maid_account_holder_(maid_account_holder),
      metadata_manager_(metadata_manager),
      pmid_account_holder_(pmid_account_holder),
      data_holder_(data_holder) {}

void Demultiplexer::HandleMessage(const std::string& serialised_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    switch (message.inner_message_type()) {
      case nfs::DataMessage::message_type_identifier: {
        nfs::DataMessage data_message(message.serialised_inner_message<nfs::DataMessage>());
        return HandleDataMessagePersona(data_message, reply_functor);
      }
      case nfs::GenericMessage::message_type_identifier: {
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
      return HandleDataType<MetadataManager>(data_message, reply_functor, metadata_manager_);
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
          request_data_message.action(),
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
    case maidsafe::detail::DataTagValue::kAnmidValue:
      return data_holder_.GetFromCache<passport::PublicAnmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnsmidValue:
      return data_holder_.GetFromCache<passport::PublicAnsmid>(data_message);
    case maidsafe::detail::DataTagValue::kAntmidValue:
      return data_holder_.GetFromCache<passport::PublicAntmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnmaidValue:
      return data_holder_.GetFromCache<passport::PublicAnmaid>(data_message);
    case maidsafe::detail::DataTagValue::kMaidValue:
      return data_holder_.GetFromCache<passport::PublicMaid>(data_message);
    case maidsafe::detail::DataTagValue::kPmidValue:
      return data_holder_.GetFromCache<passport::PublicPmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnmpidValue:
      return data_holder_.GetFromCache<passport::PublicAnmpid>(data_message);
    case maidsafe::detail::DataTagValue::kMpidValue:
      return data_holder_.GetFromCache<passport::PublicMpid>(data_message);
    case maidsafe::detail::DataTagValue::kImmutableDataValue:
      return data_holder_.GetFromCache<ImmutableData>(data_message);
    case maidsafe::detail::DataTagValue::kMutableDataValue:
      return data_holder_.GetFromCache<MutableData>(data_message);
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
    case maidsafe::detail::DataTagValue::kAnmidValue:
      return data_holder_.StoreInCache<passport::PublicAnmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnsmidValue:
      return data_holder_.StoreInCache<passport::PublicAnsmid>(data_message);
    case maidsafe::detail::DataTagValue::kAntmidValue:
      return data_holder_.StoreInCache<passport::PublicAntmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnmaidValue:
      return data_holder_.StoreInCache<passport::PublicAnmaid>(data_message);
    case maidsafe::detail::DataTagValue::kMaidValue:
      return data_holder_.StoreInCache<passport::PublicMaid>(data_message);
    case maidsafe::detail::DataTagValue::kPmidValue:
      return data_holder_.StoreInCache<passport::PublicPmid>(data_message);
    case maidsafe::detail::DataTagValue::kAnmpidValue:
      return data_holder_.StoreInCache<passport::PublicAnmpid>(data_message);
    case maidsafe::detail::DataTagValue::kMpidValue:
      return data_holder_.StoreInCache<passport::PublicMpid>(data_message);
    case maidsafe::detail::DataTagValue::kImmutableDataValue:
      return data_holder_.StoreInCache<ImmutableData>(data_message);
    case maidsafe::detail::DataTagValue::kMutableDataValue:
      return data_holder_.StoreInCache<MutableData>(data_message);
    default :
      LOG(kError) << "Unhandled data type";
  }
}

}  // namespace vault

}  // namespace maidsafe
