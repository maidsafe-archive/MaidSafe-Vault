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
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/vault/data_holder/data_holder_service.h"
#include "maidsafe/vault/maid_account_holder/maid_account_holder_service.h"
#include "maidsafe/vault/metadata_manager/metadata_manager_service.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account_holder_service.h"


namespace maidsafe {

namespace vault {

namespace {

template<typename Persona>
void HandleDataType(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor,
                    Persona& persona) {
  switch (data_message.data().type) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return persona.template HandleDataMessage<data_type>(data_message, reply_functor);
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // unnamed namespace


Demultiplexer::Demultiplexer(MaidAccountHolderService& maid_account_holder_service,
                             MetadataManagerService& metadata_manager_service,
                             PmidAccountHolderService& pmid_account_holder_service,
                             DataHolderService& data_holder)
    : maid_account_holder_service_(maid_account_holder_service),
      metadata_manager_service_(metadata_manager_service),
      pmid_account_holder_service_(pmid_account_holder_service),
      data_holder_(data_holder) {}

void Demultiplexer::HandleMessage(const std::string& serialised_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    switch (message.inner_message_type()) {
      case nfs::MessageCategory::kData: {
        nfs::DataMessage data_message(message.serialised_inner_message<nfs::DataMessage>());
        return PersonaHandleMessage(data_message, reply_functor);
      }
      case nfs::MessageCategory::kGeneric: {
        nfs::GenericMessage generic_msg(message.serialised_inner_message<nfs::GenericMessage>());
        return PersonaHandleMessage(generic_msg, reply_functor);
      }
      default:
        LOG(kError) << "Unhandled inner_message_type";
    }
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message: " << ex.what();
  }
}

template<>
void Demultiplexer::PersonaHandleMessage<nfs::DataMessage>(
    const nfs::DataMessage& message,
    const routing::ReplyFunctor& reply_functor) {
  switch (message.destination_persona()) {
    case nfs::Persona::kMaidAccountHolder:
      return HandleDataType<MaidAccountHolderService>(message, reply_functor,
                                                      maid_account_holder_service_);
    case nfs::Persona::kMetadataManager:
      return HandleDataType<MetadataManagerService>(message, reply_functor,
                                                    metadata_manager_service_);
    case nfs::Persona::kPmidAccountHolder:
      return HandleDataType<PmidAccountHolderService>(message, reply_functor,
                                                      pmid_account_holder_service_);
    case nfs::Persona::kDataHolder:
      return HandleDataType<DataHolderService>(message, reply_functor, data_holder_);
    default:
      LOG(kError) << "Unhandled Persona";
  }
}

template<>
void Demultiplexer::PersonaHandleMessage<nfs::GenericMessage>(
    const nfs::GenericMessage& message,
    const routing::ReplyFunctor& reply_functor) {
  switch (message.destination_persona()) {
    case nfs::Persona::kMaidAccountHolder:
      return maid_account_holder_service_.HandleGenericMessage(message, reply_functor);
    case nfs::Persona::kMetadataManager:
      return metadata_manager_service_.HandleGenericMessage(message, reply_functor);
    case nfs::Persona::kPmidAccountHolder:
      return pmid_account_holder_service_.HandleGenericMessage(message, reply_functor);
    case nfs::Persona::kDataHolder:
      return data_holder_.HandleGenericMessage(message, reply_functor);
    default:
      LOG(kError) << "Unhandled Persona";
  }
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
          request_data_message.destination_persona(),
          request_data_message.source(),
          nfs::DataMessage::Data(request_data_message.data().type,
                                 request_data_message.data().name,
                                 cached_content,
                                 request_data_message.data().action));
      nfs::Message response_message(nfs::DataMessage::message_type_identifier,
                                    response_data_message.Serialise().data);
      serialised_message = response_message.Serialise()->string();
      return true;
    }
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling get from cache request: " << ex.what();
  }
  return false;
}

NonEmptyString Demultiplexer::HandleGetFromCache(const nfs::DataMessage& data_message) {
  switch (data_message.data().type) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return data_holder_.GetFromCache<data_type>(data_message);
    }
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
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return data_holder_.StoreInCache<data_type>(data_message);
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // namespace vault

}  // namespace maidsafe
