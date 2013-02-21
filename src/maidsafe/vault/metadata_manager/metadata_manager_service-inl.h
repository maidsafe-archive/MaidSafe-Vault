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
#include "maidsafe/vault/metadata_manager/metadata_pb.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MetadataManagerService::HandleDataMessage(const nfs::DataMessage& data_message,
                                        const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(data_message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  switch (data_message.data().action) {
    case nfs::DataMessage::Action::kPut:
      return HandlePut<Data>(data_message, reply_functor);
    case nfs::DataMessage::Action::kGet:
      return HandleGet<Data>(data_message, reply_functor);
    case nfs::DataMessage::Action::kDelete:
      return HandleDelete<Data>(data_message, reply_functor);
    default: {
      reply = nfs::Reply(VaultErrors::operation_not_supported, data_message.Serialise().data);
      std::lock_guard<std::mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(data_message, reply.error());
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<typename Data>
void MetadataManagerService::HandlePut(const nfs::DataMessage& data_message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
    ValidatePutSender(data_message);
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    auto data_name(data.name());
    auto data_size(static_cast<int32_t>(data_message.data().content.string().size()));
    metadata_handler_.template IncrementSubscribers<Data>(data_name, data_size);
    on_scope_exit strong_guarantee([this, data_name] {
       try {
         metadata_handler_.template DecrementSubscribers<Data>(data_name);
       }
       catch(...) {}
    });
    if (AddResult(data_message, reply_functor, MakeError(CommonErrors::success))) {
      PmidName target_data_holder(routing_.ClosestToID(data_message.source().node_id) ?
                                  data_message.data_holder() :
                                  Identity(routing_.RandomConnectedNode().string()));
      Put(data, target_data_holder);
    }
    strong_guarantee.Release();
  }
  catch(const maidsafe_error& error) {
    AddResult(data_message, reply_functor, error);
  }
  catch(...) {
    AddResult(data_message, reply_functor, MakeError(CommonErrors::unknown));
  }
}

template<typename Data>
void MetadataManagerService::Put(const Data& data, const PmidName& target_data_holder) {
  auto put_op(std::make_shared<nfs::PutOrDeleteOp>(
      kPutRepliesSuccessesRequired_,
      [this, account_name, data_name, reply_functor](nfs::Reply overall_result) {
          HandlePutResult<Data>(overall_result, account_name, data_name, reply_functor,
                                is_unique_on_network<Data>());
      }));
  nfs_.Put(target_data_holder,
           data,
           [put_op](std::string serialised_reply) {
               nfs::HandlePutOrDeleteReply(put_op, serialised_reply);
           });
}

template<typename Data>
void MetadataManagerService::HandleGet(nfs::DataMessage data_message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateGetSender(data_message);
    Data::name_type data_name(Identity(data_message.data().name));
    auto online_holders(metadata_handler_.GetOnlineDataHolders<Data>(data_name));
    auto get_handler(std::make_shared(GetHandler<Data>(reply_functor, online_holders.size(),
                                                       data_message.message_id())));


    for (auto& online_holder : online_holders) {
      nfs_.Get(online_holder, data_name, [this, get_handler](std::string serialised_reply) {
                                             this->OnHandleGet(get_handler, serialised_reply);
                                         });
    }
  }
  catch(const std::exception& e) {
    LOG(kWarning) << "Error during Get: " << e.what();
  }
}

template<typename Data>
void MetadataManagerService::OnHandleGet(std::shared_ptr<GetHandler<Data>> get_handler,
                                         const std::string& serialised_reply) {
  protobuf::DataOrProof data_or_proof;
  try {
    nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
    if (reply.IsSuccess()) {
      if (!data_or_proof.ParseFromString(reply.data()) || !data_or_proof.has_serialised_data())
        ThrowError(CommonErrors::parsing_error);
      protobuf::DataOrProof::Data proto_data;
      if (!proto_data.ParseFromString(data_or_proof.serialised data()))
        ThrowError(CommonErrors::parsing_error);
      if (proto_data.type() != static_cast<int>(Data::name_type::tag_type::kEnumValue))
        ThrowError(CommonErrors::parsing_error);
      Data data(typename Data::name_type(Identity(proto_data.name())),
                typename Data::serialised_type(NonEmptyString(proto_data.content())));
      std::lock_guard<std::mutex> lock(get_handler->mutex);
      on_scope_exit strong_guarantee(on_scope_exit::RevertValue(get_handler->reply_functor));
      if (get_handler->reply_functor) {
        reply = nfs::Reply(CommonErrors::success, data.Serialise()->string());
        get_handler->reply_functor(reply.Serialise()->string());
        get_handler->reply_functor = nullptr;
      }
      if (!get_handler->validation_result.IsInitialised()) {
        get_handler->validation_result =
            crypto::Hash<crypto::SHA512>(proto_data.content() + get_handler->message_id->string());
      }
      // TODO(Fraser) - Possibly check our own hash function here in case our binary is broken.
      strong_guarantee.Release();
    } else if (reply.error() == MakeError(VaultErrors::data_available_not_given).code()) {
      if (!data_or_proof.ParseFromString(reply.data()) || !data_or_proof.has_serialised_data())
        ThrowError(CommonErrors::parsing_error);
    }
  }
  catch(...) {
    --get_handler->holder_count;
    data_or_proof.Clear();
  }
  std::lock_guard<std::mutex> lock(get_handler->mutex);
  get_handler->data_holder_results.push_back(data_or_proof);
  if (get_handler->reply_functor &&
      get_handler->data_holder_results.size() == get_handler->holder_count) {
    // The overall operation has not returned the data.  Calculate whether any holders have it.
    auto itr(std::find_if(std::begin(get_handler->data_holder_results),
                          std::end(get_handler->data_holder_results),
                          [](const protobuf::DataOrProof& data_or_proof){
                            return data_or_proof.IsInitialized();
                          }));
    if (itr == std::end(get_handler->data_holder_results)) {
      get_handler->reply_functor(nfs::Reply(CommonErrors::no_such_element).Serialise()->string());
    } else {
      get_handler->reply_functor(
          nfs::Reply(VaultErrors::data_available_not_given).Serialise()->string());
    }
  }
  if(get_handler->data_holder_results.size() == get_handler->holder_count)
    IntegrityCheck(get_handler);
}

void IntegrityCheck(std::shared_ptr<GetHandler<Data>> get_handler) {
  std::lock_guard<std::mutex> lock(get_handler->mutex);
  for (auto& result: get_handler->data_holder_results) {

  }

  // TODO(David) - BEFORE_RELEASE - All machinery in place here for handling integrity checks.
}

template<typename Data>
void MetadataManagerService::HandleDelete(const nfs::DataMessage& data_message,
                                          const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateDeleteSender(data_message);
    // wait for 3 requests

    typename Data::name_type data_name(data_message.data().name);
    metadata_handler_.template DecrementSubscribers<Data>(data.name());
    // Decrement should send delete to PMID's on event data is actually deleted
    SendReply(data_message, MakeError(CommonErrors::success), reply_functor);
  }


}

template<typename Data>
void MetadataManagerService::HandlePutResult(const nfs::Reply& overall_result) {
  if (overall_result.IsSuccess())
    return;

  try {
    nfs::DataMessage original_message(nfs::DataMessage::serialised_type(overall_result.data()));
    if (!ThisVaultInGroupForData(original_message)) {
      LOG(kInfo) << "Stopping retries for Put, since no longer responsible for this data.";
      return;
    }

    Data data(typename Data::name_type(original_message.data().name),
              typename Data::serialised_type(original_message.data().content));
    Put(data, Identity(routing_.RandomConnectedNode().string()));
  }
  catch(const std::exception& e) {
    LOG(kError) << "Error retrying Put: " << e.what();
  }
}

template<typename Data>
void MetadataManagerService::HandleGetReply(std::string serialised_reply) {
  try {
    nfs::Reply reply((Reply::serialised_type(NonEmptyString(serialised_reply))));
    // if failure - try another DH?
  }
  //catch(const maidsafe_error& error) {
  //  LOG(kWarning) << "nfs error: " << error.code() << " - " << error.what();
  //  op->HandleReply(Reply(error));
  //}
  //catch(const std::exception& e) {
  //  LOG(kWarning) << "nfs error: " << e.what();
  //  op->HandleReply(Reply(CommonErrors::unknown));
  //}
}

template<typename Data>
void MetadataManagerService::OnGenericErrorHandler(nfs::GenericMessage /*generic_message*/) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_INL_H_
