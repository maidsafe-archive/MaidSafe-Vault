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

#include "maidsafe/nfs/message.h"
#include "maidsafe/vault/maid_account_holder.h"
#include "maidsafe/vault/meta_data_manager.h"
#include "maidsafe/vault/pmid_account_holder.h"
#include "maidsafe/vault/data_holder.h"

namespace maidsafe {

namespace vault {

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

    HandleMessageType(message, reply_functor);
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message : " << ex.what();
  }
}

void Demultiplexer::HandleMessageType(nfs::Message& message,
                                      const routing::ReplyFunctor& reply_functor) {
  switch (message.destination_persona_type()) {
    case nfs::PersonaType::kMaidAccountHolder :
      maid_account_holder_.HandleMessage(message, reply_functor);
      break;
    case nfs::PersonaType::kMetaDataManager :
      metadata_manager_.HandleMessage(message, reply_functor);
      break;
    case nfs::PersonaType::kPmidAccountHolder :
      pmid_account_holder_.HandleMessage(message, reply_functor);
      break;
    case nfs::PersonaType::kDataHolder :
      data_holder_.HandleMessage(message, reply_functor);
      break;
    default :
      LOG(kError) << "Unhandled personatype";
  }
}

bool Demultiplexer::HaveCache(std::string& serialised_message) {
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
  if (message.destination_persona_type() == nfs::PersonaType::kDataHolder) {
    return data_holder_.HaveCache(message);
  } else {
    LOG(kError) << "Unhandled personatype for cache request";
    return false;
  }
}

void Demultiplexer::StoreCache(const std::string& serialised_message) {
  try {
    nfs::Message message((nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    HandleStoreCache(message);
  } catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling store cache request : " << ex.what();
  }
}

void Demultiplexer::HandleStoreCache(const nfs::Message& message) {
  if (message.destination_persona_type() == nfs::PersonaType::kDataHolder)
    return data_holder_.StoreCache(message);
  else
    LOG(kError) << "Unhandled personatype for store cache request";
}

}  // namespace vault

}  // namespace maidsafe
