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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>
#include <vector>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MetadataManagerService::HandleDataMessage(const nfs::DataMessage& data_message,
                                        const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  if (accumulator_.CheckHandled(data_message, reply))
    return reply_functor(reply.Serialise()->string());

  if (data_message.data().action == nfs::DataMessage::Action::kGet &&
      ValidateGetSender(data_message)) {
    HandleGet<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kPut &&
           ValidateMAHSender(data_message)) {
    HandlePut<Data>(data_message, reply_functor);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete &&
             ValidateMAHSender(data_message)) {
    HandleDelete<Data>(data_message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported, data_message.Serialise().data);
    accumulator_.SetHandled(data_message, reply.error());
    reply_functor(reply.Serialise()->string());
  }
}


template<typename Data>
void MetadataManagerService::HandlePut(const nfs::DataMessage& data_message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    auto data_name(data.name());
    on_scope_exit strong_guarantee([this, data] {
                   try {
                        metadata_handler_.template DecrementSubscribers<Data>(data.name());
                   } catch(...) { }
}
                                   );
    metadata_handler_.IncrementSubscribers<Data>(data.name(),
                                           data_message.data().content.string().size());
    AddResult(data_message, reply_functor, MakeError(CommonErrors::success));
    strong_guarantee.Release();
  }
  catch(const maidsafe_error& error) {
  try {
    AddResult(data_message, reply_functor, error);
  } catch(...) {}
  }
  catch(...) {
    try {
       AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown));
     } catch(...) {}
  }
}

template<typename Data>
void MetadataManagerService::StoreData(const nfs::DataMessage& data_message,
               const routing::ReplyFunctor& reply_functor) {
  try {

    if (routing_.ClosestToID(data_message.source().node_id)) {
      nfs_.Put(data,
               data_message.data_holder(),
               nullptr);  // Do we care if this did not work
      // or should we check we have more than 2 pmids and retry ?
    } else {
      nfs_.Put(data,
               routing_.RandomConnectedNode(),
               nullptr);
    }
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
  }
}

template<typename Data>
void MetadataManagerService::HandleGet(nfs::DataMessage data_message,
                                       const routing::ReplyFunctor& reply_functor) {

  nfs_.Get( metadata_handler_.GetOnlinePmid(Identity(data_message.data().name),
                                            reply_functor));


//  reply_functor(nfs::Reply(NfsErrors::failed_to_get_data).Serialise()->string());
}

template<typename Data>
void MetadataManagerService::HandleDelete(const nfs::DataMessage& data_message,
                                          const routing::ReplyFunctor& reply_functor) {
  Data data(typename Data::name_type(data_message.data().name),
            typename Data::serialised_type(data_message.data().content));
  metadata_handler_.template DecrementSubscribers<Data>(data.name());
  // Decrement should send delete to PMID's on event data is actually deleted
  SendReply(data_message, MakeError(CommonErrors::success), reply_functor);
}


// These are probably not required any more !!!!!!!!!!!!!!!!!1
// On error handler's
template<typename Data>
void MetadataManagerService::OnPutErrorHandler(nfs::DataMessage data_message) {
  if (routing::GroupRangeStatus::kInRange ==
      routing_.IsNodeIdInGroupRange(data_message.source().node_id)) {
    nfs_.Put<Data>(data_message,
         [this](nfs::DataMessage data_msg) { this->OnPutErrorHandler<Data>(data_msg); });
  }
}

template<typename Data>
void MetadataManagerService::OnDeleteErrorHandler(nfs::DataMessage data_message) {
  if (routing::GroupRangeStatus::kInRange ==
      routing_.IsNodeIdInGroupRange(data_message.source().node_id)) {
    nfs_.Delete<Data>(data_message,
        [this](nfs::DataMessage data_msg) { this->OnDeleteErrorHandler<Data>(data_msg); });
  }
}

template<typename Data>
void MetadataManagerService::OnGenericErrorHandler(nfs::GenericMessage /*generic_message*/) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_INL_H_
