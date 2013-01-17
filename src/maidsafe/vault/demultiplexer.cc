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

#include "maidsafe/vault/data_holder/data_holder.h"
#include "maidsafe/vault/maid_account_holder/maid_account_holder.h"
#include "maidsafe/vault/metadata_manager/metadata_manager.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account_holder.h"


namespace maidsafe {

namespace vault {

namespace {

template<typename PersonaType>
void HandleDataType(nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor,
                    PersonaType& persona_type) {
  // static assert
  switch (message.data_type()) {
    case maidsafe::detail::DataTagValue::kAnmidValue:
      persona_type.template HandleMessage<passport::PublicAnmid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kAnsmidValue:
      persona_type.template HandleMessage<passport::PublicAnsmid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kAntmidValue:
      persona_type.template HandleMessage<passport::PublicAntmid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kAnmaidValue:
      persona_type.template HandleMessage<passport::PublicAnmaid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kMaidValue:
      persona_type.template HandleMessage<passport::PublicMaid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kPmidValue:
      persona_type.template HandleMessage<passport::PublicPmid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kMidValue:
      persona_type.template HandleMessage<passport::Mid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kSmidValue:
      persona_type.template HandleMessage<passport::Smid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kTmidValue:
      persona_type.template HandleMessage<passport::Tmid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kAnmpidValue:
      persona_type.template HandleMessage<passport::PublicAnmpid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kMpidValue:
      persona_type.template HandleMessage<passport::PublicMpid>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kImmutableDataValue:
      persona_type.template HandleMessage<ImmutableData>(data_message, reply_functor);
      break;
    case maidsafe::detail::DataTagValue::kMutableDataValue:
      persona_type.template HandleMessage<MutableData>(data_message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled data type";
  }
}

}  // unnamed namespace


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
//    HandleMessagePersonaType(message, reply_functor);
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message: " << ex.what();
  }
}

void Demultiplexer::HandleDataMessagePersonaType(nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  switch (data_message.destination_persona_type()) {
    case nfs::PersonaType::kMaidAccountHolder :
      HandleDataType<MaidAccountHolder>(data_message, reply_functor, maid_account_holder_);
      break;
    case nfs::PersonaType::kMetadataManager :
      HandleDataType<MetadataManager>(data_message, reply_functor, metadata_manager_);
      break;
    case nfs::PersonaType::kPmidAccountHolder :
      HandleDataType<PmidAccountHolder>(data_message, reply_functor, pmid_account_holder_);
      break;
    case nfs::PersonaType::kDataHolder :
      HandleDataType<DataHolder>(data_message, reply_functor, data_holder_);
      break;
    default :
      LOG(kError) << "Unhandled PersonaType";
  }
}

void Demultiplexer::HandleGenericMessagePersonaType(nfs::GenericMessage& /*generic_message*/,
                                                    const routing::ReplyFunctor& /*reply_functor*/) {
}

bool Demultiplexer::GetFromCache(std::string& serialised_message) {
  try {
    nfs::Message request_message(
        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::DataMessage request_data_message(
        (request_message.serialised_inner_message<DataMessage>()));
    auto cached_content(HandleGetFromCache(request_data_message));
    if (cached_content.IsInitialised()) {
      nfs::DataMessage response_data_message(request_data_message.action_type(),
                                             request_data_message.destination_persona_type(),
                                             request_data_message.source(),
                                             request_data_message.data_type(),
                                             request_data_message.name(),
                                             request_data_message);
      Message respons_message(DataMessage::message_type_identifier,
                              response_data_message.Serialise().data, asymm::Signature());
      serialised_message = respons_message.Serialise()->string();
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
    default :
      LOG(kError) << "Unhandled data type";
  }
  return NonEmptyString();
}

void Demultiplexer::StoreInCache(const std::string& serialised_message) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::DataMessage data_message((message.serialised_inner_message<DataMessage>()));
    HandleStoreInCache(data_message);
  } catch(const std::exception& ex) {
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
