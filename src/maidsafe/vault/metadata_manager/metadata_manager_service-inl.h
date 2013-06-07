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
#include <memory>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/maid_account_holder/maid_account.pb.h"
#include "maidsafe/vault/metadata_manager/metadata_helpers.h"
#include "maidsafe/vault/metadata_manager/metadata.pb.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/unresolved_element.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data, nfs::MessageAction Action>
MetadataUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                              const MetadataValue& metadata_value,
                                              const NodeId& this_id) {
  static_assert(Action == nfs::MessageAction::kPut || Action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  assert(message.data().type);
  return MetadataUnresolvedEntry(
      std::make_pair(GetDataNameVariant(DataTagValue(message.data().type.get()),
                                        Identity(message.data().name)), Action),
      metadata_value, this_id);
}

void SendMetadataCost(const nfs::Message& original_message,
                      const routing::ReplyFunctor& reply_functor,
                      nfs::Reply& reply);

template<typename Accumulator>
bool AccumulateMetadataPut(const nfs::Message& message,
                           const routing::ReplyFunctor& reply_functor,
                           const nfs::Reply& reply,
                           Accumulator& accumulator,
                           std::mutex& accumulator_mutex,
                           int requests_required) {
  std::vector<typename Accumulator::PendingRequest> pending_requests;
  nfs::Reply overall_reply(CommonErrors::success);

  //FIXME(Prakash): Implement merge cost here
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex);
    auto pending_results(accumulator.PushSingleResult(message, reply_functor, reply));
    if (static_cast<int>(pending_results.size()) < requests_required)
      return false;

    auto result(nfs::GetSuccessOrMostFrequentReply(pending_results, requests_required));
    if (!result.second && pending_results.size() < routing::Parameters::node_group_size)
      return false;
    maidsafe_error overall_return_code((*result.first).error());
    if (overall_return_code.code() == CommonErrors::success) {
      NonEmptyString serialised_cost;
      overall_reply = nfs::Reply(CommonErrors::success, serialised_cost);
    } else {
      overall_reply = nfs::Reply(overall_return_code); // original message to be appended in reply
    }
    pending_requests = accumulator.SetHandled(message, nfs::Reply(overall_return_code));
  }

  for (auto& pending_request : pending_requests)
    SendMetadataCost(pending_request.msg, pending_request.reply_functor, overall_reply);

  return true;
}

}  // namespace detail

template<typename Data>
MetadataManagerService::GetHandler<Data>::GetHandler(const routing::ReplyFunctor& reply_functor_in,
                                                     size_t holder_count_in,
                                                     const nfs::MessageId& message_id_in)
    : reply_functor(reply_functor_in),
      holder_count(holder_count_in),
      message_id(message_id_in),
      mutex(),
      validation_result(),
      data_holder_results() {}


template<typename Data>
void MetadataManagerService::HandleMessage(const nfs::Message& message,
                                           const routing::ReplyFunctor& reply_functor) {
  nfs::Reply reply(CommonErrors::success);
  // FIXME Prakash validate sender
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  switch (message.data().action) {
    case nfs::MessageAction::kPut:
      return HandlePut<Data>(message, reply_functor);
    case nfs::MessageAction::kPutResult:
      return HandlePutResult<Data>(message);
    case nfs::MessageAction::kGet:
      return HandleGet<Data>(message, reply_functor);
    case nfs::MessageAction::kDelete:
      return HandleDelete<Data>(message, reply_functor);
    case nfs::MessageAction::kSynchronise:
    case nfs::MessageAction::kAccountTransfer:
      return HandleSync(message);
    default: {
      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
      std::lock_guard<std::mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, reply);
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<typename Data>
void MetadataManagerService::HandlePut(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
    ValidatePutSender(message);
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    auto data_name(data.name());
    auto data_size(static_cast<int32_t>(message.data().content.string().size()));
    auto is_duplicate_and_cost(metadata_handler_.template CheckPut<Data>(data_name, data_size));
    protobuf::Cost proto_cost;
    proto_cost.set_cost(is_duplicate_and_cost.second);
    assert(proto_cost.IsInitialized());
    nfs::Reply reply(CommonErrors::success, NonEmptyString(proto_cost.SerializeAsString()));
    if (detail::AccumulateMetadataPut(message, reply_functor, reply, accumulator_,
                                      accumulator_mutex_, kPutRequestsRequired_)) {
      if (is_duplicate_and_cost.first) {  // No need to store data on DH
        MetadataValue metadata_value(data_size);
        AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message, metadata_value);
      } else {
        PmidName target_data_holder(routing_.ClosestToId(message.source().node_id) ?
                                    message.data_holder() :
                                    Identity(routing_.RandomConnectedNode().string()));
        // Account will be created by PutResult message from PAH
        Put(data, target_data_holder);
      }
    }
  }
  catch(const maidsafe_error& error) {
    detail::AccumulateMetadataPut(message, reply_functor, nfs::Reply(error), accumulator_,
                                  accumulator_mutex_, kPutRequestsRequired_);
  }
  catch(...) {
    detail::AccumulateMetadataPut(message, reply_functor, nfs::Reply(CommonErrors::unknown),
                                  accumulator_, accumulator_mutex_, kPutRequestsRequired_);
  }
}


// FIXME Prakash remove function !!!
template<typename Data>
void MetadataManagerService::Put(const Data& data, const PmidName& target_data_holder) {
  nfs_.Put(target_data_holder, data, nullptr);
}

template<typename Data>
void MetadataManagerService::HandlePutResult(const nfs::Message& message) {
  try {
    ValidatePutResultSender(message);
    // FIXME: Need to adjust accumulator to identify/accumulate message from PAHs of a given DH
    if (detail::AddResult(message, nullptr, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kPutRequestsRequired_)) {
      protobuf::PutResult proto_put_result;
      if (!proto_put_result.ParseFromString(message.data().content.string()))
        ThrowError(CommonErrors::parsing_error);
      if (proto_put_result.result()) {
        // Create record
        int32_t data_size(proto_put_result.data_size());
        MetadataValue metadata_value(data_size);
        AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message, metadata_value);
      } else {
        // FIXME need a record failure nodes vector to workout when we need to retry on different data holder
        Data data(typename Data::name_type(message.data().name),
                  typename Data::serialised_type(
                    NonEmptyString(proto_put_result.serialised_data())));
        PmidName target_data_holder(Identity(routing_.RandomConnectedNode().string()));
        Put(data, target_data_holder);
      }
    }
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "Error during HandlePutResult: " << error.what();
  }
}

template<typename Data>
void MetadataManagerService::HandleGet(nfs::Message message,
                                       const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateGetSender(message);
    typename Data::name_type data_name(Identity(message.data().name));
    auto online_holders(metadata_handler_.GetOnlineDataHolders<Data>(data_name));
    std::shared_ptr<GetHandler<Data>> get_handler(new GetHandler<Data>(reply_functor,
                                                                       online_holders.size(),
                                                                       message.message_id()));
    for (auto& online_holder : online_holders) {
      nfs_.Get<Data>(online_holder,
                     data_name,
                     [this, get_handler] (std::string serialised_reply) {
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
      if (!data_or_proof.ParseFromString(reply.data().string()) ||
          !data_or_proof.has_serialised_data())
        ThrowError(CommonErrors::parsing_error);
      protobuf::DataOrProof::Data proto_data;
      if (!proto_data.ParseFromString(data_or_proof.serialised_data()))
        ThrowError(CommonErrors::parsing_error);
      if (proto_data.type() != static_cast<int>(Data::type_enum_value()))
        ThrowError(CommonErrors::parsing_error);
      Data data(typename Data::name_type(Identity(proto_data.name())),
                typename Data::serialised_type(NonEmptyString(proto_data.content())));
      std::lock_guard<std::mutex> lock(get_handler->mutex);
      on_scope_exit strong_guarantee(on_scope_exit::RevertValue(get_handler->reply_functor));
      if (get_handler->reply_functor) {
        reply = nfs::Reply(CommonErrors::success, data.Serialise().data);
        get_handler->reply_functor(reply.Serialise()->string());
        get_handler->reply_functor = nullptr;
      }
      if (!get_handler->validation_result.IsInitialised()) {
        get_handler->validation_result =
            crypto::Hash<crypto::SHA512>(proto_data.content() + get_handler->message_id->string());
      }
      // TODO(Fraser) - Possibly check our own hash function here in case our binary is broken.
      strong_guarantee.Release();
    } else if (reply.error().code() == MakeError(VaultErrors::data_available_not_given).code()) {
      if (!data_or_proof.ParseFromString(reply.data().string()) ||
          !data_or_proof.has_serialised_data())
        ThrowError(CommonErrors::parsing_error);
    }
  }
  catch(...) {
    std::lock_guard<std::mutex> lock(get_handler->mutex);
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
                          [] (const protobuf::DataOrProof& data_or_proof){
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

template<typename Data>
void MetadataManagerService::IntegrityCheck(std::shared_ptr<GetHandler<Data>> /*get_handler*/) {
//  std::lock_guard<std::mutex> lock(get_handler->mutex);
////  for (auto& result: get_handler->data_holder_results) {

////  }

//  // TODO(David) - BEFORE_RELEASE - All machinery in place here for handling integrity checks.
}

template<typename Data>
void MetadataManagerService::HandleDelete(const nfs::Message& message,
                                          const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateDeleteSender(message);
    // wait for 3 requests

    typename Data::name_type data_name(message.data().name);
    metadata_handler_.template DecrementSubscribers<Data>(data_name);
    // Decrement should send delete to PMID's on event data is actually deleted
    detail::SendReply(message, MakeError(CommonErrors::success), reply_functor);
  }
  catch (...) {}

}

//TODO(Prakash) Change this to service to handle data stored/ not stored message and then sync
template<typename Data>
void MetadataManagerService::HandlePutResult(const nfs::Reply& overall_result) {
  if (overall_result.IsSuccess())
    return;

  try {
    nfs::Message original_message(nfs::Message::serialised_type(overall_result.data()));
    if (!ThisVaultInGroupForData(original_message)) {
      LOG(kInfo) << "Stopping retries for Put, since no longer responsible for this data.";
      return;
    }

    Data data(typename Data::name_type(original_message.data().name),
              typename Data::serialised_type(original_message.data().content));
    Put(data, PmidName(Identity(routing_.RandomConnectedNode().string())));
  }
  catch(const std::exception& e) {
    LOG(kError) << "Error retrying Put: " << e.what();
  }
}

template<typename Data>
void MetadataManagerService::HandleGetReply(std::string serialised_reply) {
  try {
    nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
    // if failure - try another DH?
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "nfs error: " << error.code() << " - " << error.what();
  }
  //catch(const std::exception& e) {
  //  LOG(kWarning) << "nfs error: " << e.what();
  //  op->HandleReply(Reply(CommonErrors::unknown));
  //}
}

template<typename Data>
void MetadataManagerService::OnGenericErrorHandler(nfs::Message /*message*/) {}

template<typename Data, nfs::MessageAction Action>
void MetadataManagerService::AddLocalUnresolvedEntryThenSync(
    const nfs::Message& message,
    const MetadataValue& metadata_value) {
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, Action>(message, metadata_value,
                                                                    routing_.kNodeId()));
  metadata_handler_.AddLocalUnresolvedEntry(unresolved_entry);
  typename Data::name_type data_name(message.data().name);
  Sync<Data>(data_name);
}

// =============== Sync ============================================================================

template<typename Data>
void MetadataManagerService::Sync(const typename Data::name_type& data_name) {
  auto serialised_sync_data(metadata_handler_.GetSyncData<Data>(data_name));
  if (!serialised_sync_data.IsInitialised())  // Nothing to sync
    return;

  nfs_.Sync<Data>(data_name, serialised_sync_data);
  // TODO(Fraser#5#): 2013-05-03 - Check this is correct place to increment sync attempt counter.
  metadata_handler_.IncrementSyncAttempts<Data>(data_name);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_INL_H_
