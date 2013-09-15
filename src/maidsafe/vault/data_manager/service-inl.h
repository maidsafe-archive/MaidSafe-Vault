/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>
#include <vector>
#include <memory>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/data_manager/helpers.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/data_manager/action_add_pmid.h"
#include "maidsafe/vault/data_manager/action_remove_pmid.h"
#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_node_down.h"
#include "maidsafe/vault/data_manager/action_node_up.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data, nfs::MessageAction Action>
DataManagerUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                              const DataManagerValue& metadata_value,
                                              const NodeId& this_id) {
  static_assert(Action == nfs::MessageAction::kPut || Action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  assert(message.data().type);
  return DataManagerUnresolvedEntry(
      std::make_pair(DataManagerKey(GetDataNameVariant(DataTagValue(message.data().type.get()),
                                        Identity(message.data().name))), Action),
                                        metadata_value,
                    this_id);
}

void SendMetadataCost(const nfs::Message& original_message,
                      const routing::ReplyFunctor& reply_functor,
                      nfs::Reply& reply);

template<typename Accumulator>
bool AccumulateMetadataPut(const nfs::Message& message,
                           const routing::ReplyFunctor& reply_functor,
                           const maidsafe_error& return_code,
                           const int32_t& cost,
                           Accumulator& accumulator,
                           std::mutex& accumulator_mutex,
                           int requests_required) {
  std::vector<typename Accumulator::PendingRequest> pending_requests;
  nfs::Reply overall_reply(CommonErrors::pending_result);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex);
    auto pending_results(accumulator.PushSingleResult(message, reply_functor,
                                                      nfs::Reply(return_code)));
    if (static_cast<int>(pending_results.size()) < requests_required)
      return false;

    auto result(nfs::GetSuccessOrMostFrequentReply(pending_results, requests_required));
    if (!result.second && pending_results.size() < routing::Parameters::node_group_size)
      return false;
    maidsafe_error overall_return_code((*result.first).error());

    if (overall_return_code.code() == CommonErrors::success) {
      // Cost only at time of resolution is taken
      protobuf::Cost proto_cost;
      proto_cost.set_cost(cost);
      assert(proto_cost.IsInitialized());
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
DataManagerService::GetHandler<Data>::GetHandler(const routing::ReplyFunctor& reply_functor_in,
                                                     size_t holder_count_in,
                                                     const nfs::MessageId& message_id_in)
    : reply_functor(reply_functor_in),
      holder_count(holder_count_in),
      message_id(message_id_in),
      mutex(),
      validation_result(),
      pmid_node_results() {}

// Commented by Mahmoud on 9 Sep.
//template<typename Data>
//void DataManagerService::HandleMessage(const nfs::Message& message,
//                                           const routing::ReplyFunctor& reply_functor) {
//  nfs::Reply reply(CommonErrors::success);
//  // FIXME Prakash validate sender
//  {
//    std::lock_guard<std::mutex> lock(accumulator_mutex_);
//    if (accumulator_.CheckHandled(message, reply))
//      return reply_functor(reply.Serialise()->string());
//  }

//  switch (message.data().action) {
//    case nfs::MessageAction::kPut:
//      return HandlePut<Data>(message, reply_functor);
//    case nfs::MessageAction::kPutResult:
//      return HandlePutResult<Data>(message);
//    case nfs::MessageAction::kGet:
//      return HandleGet<Data>(message, reply_functor);
//    case nfs::MessageAction::kDelete:
//      return HandleDelete<Data>(message);
//    case nfs::MessageAction::kStateChange:
//      return HandleStateChange<Data>(message);
//    case nfs::MessageAction::kSynchronise:
//      return HandleSync(message);
//    case nfs::MessageAction::kAccountTransfer:
//      return HandleRecordTransfer(message);
//    default: {
//      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
//      std::lock_guard<std::mutex> lock(accumulator_mutex_);
//      accumulator_.SetHandled(message, reply);
//      reply_functor(reply.Serialise()->string());
//    }
//  }
//}

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromMaidNodeToDataManager& message,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
}

template<>
void DataManagerService::HandleMessage(
   const nfs::PutResponseFromPmidManagerToDataManager& message,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Sender& sender,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Receiver& receiver) {
  typedef nfs::PutResponseFromPmidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService,
                          MessageType,
                          nfs::DataManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::DataManagerServiceMessages>::AddRequestChecker(RequiredRequests(sender)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<typename Data>
void DataManagerService::HandlePutResult(const nfs::Message& message) {
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
        DataManagerValue metadata_value(data_size);
        AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message, metadata_value);
      } else {
        // FIXME need a record failure nodes vector to workout when we need to retry on different data holder
        Data data(typename Data::Name(message.data().name),
                  typename Data::serialised_type(
                    NonEmptyString(proto_put_result.serialised_data())));
        PmidName target_pmid_node(Identity(routing_.RandomConnectedNode().string()));
        nfs_.Put(target_pmid_node, data, nullptr);
      }
    }
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "Error during HandlePutResult: " << error.what();
  }
}

template<typename Data>
void DataManagerService::HandleGet(nfs::Message message,
                                   const routing::ReplyFunctor& reply_functor) {
  try {
    ValidateGetSender(message);
    typename Data::Name data_name(Identity(message.data().name));
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
void DataManagerService::OnHandleGet(std::shared_ptr<GetHandler<Data>> get_handler,
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
      if (proto_data.type() != static_cast<uint32_t>(Data::type_enum_value()))
        ThrowError(CommonErrors::parsing_error);
      Data data(typename Data::Name(Identity(proto_data.name())),
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
  get_handler->pmid_node_results.push_back(data_or_proof);
  if (get_handler->reply_functor &&
      get_handler->pmid_node_results.size() == get_handler->holder_count) {
    // The overall operation has not returned the data.  Calculate whether any holders have it.
    auto itr(std::find_if(std::begin(get_handler->pmid_node_results),
                          std::end(get_handler->pmid_node_results),
                          [] (const protobuf::DataOrProof& data_or_proof){
                            return data_or_proof.IsInitialized();
                          }));
    if (itr == std::end(get_handler->pmid_node_results)) {
      get_handler->reply_functor(nfs::Reply(CommonErrors::no_such_element).Serialise()->string());
    } else {
      get_handler->reply_functor(
          nfs::Reply(VaultErrors::data_available_not_given).Serialise()->string());
    }
  }
  if(get_handler->pmid_node_results.size() == get_handler->holder_count)
    IntegrityCheck(get_handler);
}

template<typename Data>
void DataManagerService::IntegrityCheck(std::shared_ptr<GetHandler<Data>> /*get_handler*/) {
//  std::lock_guard<std::mutex> lock(get_handler->mutex);
////  for (auto& result: get_handler->pmid_node_results) {

////  }

//  // TODO(David) - BEFORE_RELEASE - All machinery in place here for handling integrity checks.
}

template<typename Data>
void DataManagerService::HandleDelete(const nfs::Message& message) {
  try {
    ValidateDeleteSender(message);
    // wait for 3 requests
    if (detail::AddResult(message, nullptr, MakeError(CommonErrors::success),
                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired_)) {
      DataManagerValue metadata_value(-1); // TODO(Prakash) is data size useful for delete ops?
      AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kDelete>(message, metadata_value);
      // After merge, decrement should send delete to PMID's on event data is actually deleted
    }
  }
  catch (...) {
    LOG(kWarning) << "Error during HandleDelete: ";
  }
}

template<typename Data>
void DataManagerService::HandleGetReply(std::string serialised_reply) {
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
void DataManagerService::HandleStateChange(const nfs::Message& message) {
  try {
    //  ValidateStateChangeSender(message);
    //  Construct message id based on first half of Pmid name and second half of data name
    if (detail::AddResult(message, nullptr, MakeError(CommonErrors::success), accumulator_,
                          accumulator_mutex_, kStateChangesRequired_)) {
      // AddLocalUnresolvedEntryThenSync<Data, nfs::kStateChange::kPut>(message, metadata_value);

      // check db + unresolved to work out total online pmids
      int online_holders(-1);

      if (online_holders < 3) {
        // TODO(Team): Get content.
        PmidName target_data_holder(Identity(routing_.RandomConnectedNode().string()));
        //  nfs_.Put(target_data_holder, data, nullptr);
      }
    }
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << "Error during HandleStateChange: " << error.what();
  }
}

template<typename Data>
void DataManagerService::OnGenericErrorHandler(nfs::Message /*message*/) {}

template<typename Data, nfs::MessageAction Action>
void DataManagerService::AddLocalUnresolvedEntryThenSync(
    const nfs::Message& message,
    const DataManagerValue& metadata_value) {
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, Action>(message, metadata_value,
                                                                    routing_.kNodeId()));
  metadata_handler_.AddLocalUnresolvedEntry(unresolved_entry);
  typename Data::Name data_name(message.data().name);
  Sync();
}

template<>
void DataManagerService::HandleMessage(
   const nfs::SynchroniseFromDataManagerToDataManager& message,
   const typename nfs::SynchroniseFromDataManagerToDataManager::Sender& sender,
   const typename nfs::SynchroniseFromDataManagerToDataManager::Receiver& /*receiver*/) {
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.contents->content.string()))
    ThrowError(CommonErrors::parsing_error);

  switch (static_cast<nfs::MessageAction>(proto_sync.action_type())) {
    case ActionDataManagerPut::kActionId: {
      DataManager::UnresolvedPut unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_puts_.AddUnresolvedAction(unresolved_action));
      if (resolved_action) {
        group_db_.Commit(resolved_action->key, resolved_action->action);
      }
      break;
    }
    case ActionDataManagerDelete::kActionId: {
      DataManager::UnresolvedDelete unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_deletes_.AddUnresolvedAction(unresolved_action));
      if (resolved_action)
        group_db_.Commit(resolved_action->key, resolved_action->action);
      break;
    }
    case ActionDataManagerAddPmid::kActionId: {
      DataManager::UnresolvedAddPmid unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_add_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action)
        group_db_.Commit(resolved_action->key, resolved_action->action);
      break;
    }
    case ActionDataManagerRemovePmid::kActionId: {
      DataManager::UnresolvedRemovePmid unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_remove_pmids_.AddUnresolvedAction(unresolved_action));
      if (resolved_action)
        group_db_.Commit(resolved_action->key, resolved_action->action);
      break;
    }
    case ActionDataManagerNodeUp::kActionId: {
      DataManager::UnresolvedNodeUp unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_node_ups_.AddUnresolvedAction(unresolved_action));
      if (resolved_action)
        group_db_.Commit(resolved_action->key, resolved_action->action);
      break;
    }
    case ActionDataManagerNodeDown::kActionId: {
      DataManager::UnresolvedNodeDown unresolved_action(
          proto_sync.serialised_unresolved_action(), sender.sender_id, routing_.kNodeId());
      auto resolved_action(sync_node_downs_.AddUnresolvedAction(unresolved_action));
      if (resolved_action)
        group_db_.Commit(resolved_action->key, resolved_action->action);
      break;
    }
    default: {
      assert(false);
      LOG(kError) << "Unhandled action type";
    }
  }
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DATA_MANAGER_SERVICE_INL_H_
