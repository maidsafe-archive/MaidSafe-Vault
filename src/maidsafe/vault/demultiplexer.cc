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

#include <string>

#include "maidsafe/passport/types.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/detail/data_type_values.h"
#include "maidsafe/vault/maid_account_holder.h"
#include "maidsafe/vault/meta_data_manager.h"
#include "maidsafe/vault/pmid_account_holder.h"
#include "maidsafe/vault/data_holder.h"

namespace maidsafe {

namespace vault {

namespace {
template <typename PersonaType>
void HandleDataType(nfs::Message& message,
                    const routing::ReplyFunctor& reply_functor,
                    PersonaType& persona_type) {
  //static assert
  switch (message.data_type()) {
    case detail::DataTagValue::kSmidValue:
      persona_type.template HandleMessage<passport::Smid>(message, reply_functor);
      break;
    case detail::DataTagValue::kTmidValue:
      persona_type.template HandleMessage<passport::Tmid>(message, reply_functor);
      break;
    case detail::DataTagValue::kMidValue:
      persona_type.template HandleMessage<passport::Mid>(message, reply_functor);
      break;
    case detail::DataTagValue::kAnmaidValue:
      persona_type.template HandleMessage<passport::PublicAnmaid>(message, reply_functor);
      break;
    case detail::DataTagValue::kAnmidValue:
      persona_type.template HandleMessage<passport::PublicAnmid>(message, reply_functor);
      break;
    case detail::DataTagValue::kAnsmidValue:
      persona_type.template HandleMessage<passport::PublicAnsmid>(message, reply_functor);
      break;
    case detail::DataTagValue::kAntmidValue:
      persona_type.template HandleMessage<passport::PublicAntmid>(message, reply_functor);
      break;
    case detail::DataTagValue::kMaidValue:
      persona_type.template HandleMessage<passport::PublicMaid>(message, reply_functor);
      break;
    case detail::DataTagValue::kMpidValue:
      persona_type.template HandleMessage<passport::PublicMpid>(message, reply_functor);
      break;
    case detail::DataTagValue::kImmutableDataValue:
      persona_type.template HandleMessage<ImmutableData>(message, reply_functor);
      break;
    case detail::DataTagValue::kMutableDataValue:
      persona_type.template HandleMessage<MutableData>(message, reply_functor);
      break;
    // case static_cast<int>(DataTagValue::kMessageDataValue):
    //   persona_type.template HandlePostMessage(message, reply_functor);
    //   break;
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
    data_holder_(data_holder) {
    }

void Demultiplexer::HandleMessage(const std::string& serialised_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));

    HandleMessagePersonaType(message, reply_functor);
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message : " << ex.what();
  }
}

void Demultiplexer::HandleMessagePersonaType(nfs::Message& message,
                                             const routing::ReplyFunctor& reply_functor) {
  switch (message.destination()->persona_type) {
    case nfs::PersonaType::kMaidAccountHolder :
      HandleDataType<MaidAccountHolder>(message, reply_functor, maid_account_holder_);
      break;
    case nfs::PersonaType::kMetaDataManager :
      HandleDataType<MetadataManager>(message, reply_functor, metadata_manager_);
      break;
    case nfs::PersonaType::kPmidAccountHolder :
      HandleDataType<PmidAccountHolder>(message, reply_functor, pmid_account_holder_);
      break;
    case nfs::PersonaType::kDataHolder :
      HandleDataType<DataHolder>(message, reply_functor, data_holder_);
      break;
    default :
      LOG(kError) << "Unhandled personatype";
  }
}

bool Demultiplexer::IsInCache(std::string& serialised_message) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    if (HandleHaveCache(message)) {
      serialised_message = message.Serialise().data.string();
      return true;
    }
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling cache request : " << ex.what();
  }
  return false;
}

bool Demultiplexer::HandleHaveCache(nfs::Message& message) {
  if (message.destination()->persona_type == nfs::PersonaType::kDataHolder) {
    return true; /*data_holder_.IsInCache(message);*/ // FIXME
  } else {
    LOG(kError) << "Unhandled personatype for cache request";
    return false;
  }
}

void Demultiplexer::StoreInCache(const std::string& serialised_message) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    HandleStoreCache(message);
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling store cache request : " << ex.what();
  }
}

void Demultiplexer::HandleStoreCache(const nfs::Message& message) {
  if (message.destination()->persona_type == nfs::PersonaType::kDataHolder)
    return; /*data_holder_.StoreCache(message);*/
  else
    LOG(kError) << "Unhandled personatype for store cache request";
}

}  // namespace vault

}  // namespace maidsafe
