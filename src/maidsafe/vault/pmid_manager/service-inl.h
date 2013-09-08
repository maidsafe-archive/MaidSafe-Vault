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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/message_types.h"

namespace maidsafe {
namespace vault {

namespace detail {

template<typename Data, nfs::MessageAction Action>
PmidManagerUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                                 const NodeId& this_id) {
  static_assert(Action == nfs::MessageAction::kPut || Action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return PmidManagerUnresolvedEntry(
      std::make_pair(GetDataNameVariant(DataTagValue(message.data().type.get()),
                                        Identity(message.data().name)), Action),
      static_cast<int32_t>(message.data().content.string().size()),
      this_id);
}

PmidName GetPmidAccountName(const nfs::Message& message);

}  // namespace detail


template<>
void PmidManagerService::HandleMessage(
    const nfs::PutRequestFromDataManagerToPmidManager& message,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef nfs::PutRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<MessageType,
                          nfs::PmidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>::Value()),
      [this](const MessageType& message,
             const typename MessageType::Sender& sender,
             const typename MessageType::Receiver& receiver) {
        this->HandlePut(message, sender, receiver);
      },
      accumulator_mutex_)(message, sender, receiver);
}


template<>
void PmidManagerService::HandleMessage(
    const nfs::PutResponseFromPmidNodeToPmidManager& /*message*/,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Sender& /*sender*/,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Receiver& /*receiver*/) {
}


template<>
void PmidManagerService::HandleMessage(
    const nfs::GetPmidAccountResponseFromPmidManagerToPmidNode& /*message*/,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& /*sender*/,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
}


template<>
void PmidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidManagerToPmidManager& message,
    const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Receiver& receiver) {
  typedef nfs::CreateAccountRequestFromMaidManagerToPmidManager MessageType;
  OperationHandlerWrapper<MessageType, nfs::PmidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>::Value()),
      [this](const MessageType& message,
             const typename MessageType::Sender& sender,
             const typename MessageType::Receiver& receiver) {
        this->CreatePmidAccount(message, sender, receiver);
      },
      accumulator_mutex_)(message, sender, receiver);
}

/* Commented by Mahmoud on 3 Sep. Code need refactoring
template<typename Data>
void PmidManagerService::HandleMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  ValidateDataSender(message);
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return;
  }

  if (message.data().action == nfs::MessageAction::kPut) {
    HandlePut<Data>(message);
  } else if (message.data().action == nfs::MessageAction::kDelete) {
    HandleDelete<Data>(message, reply_functor);
  } else {
    LOG(kError) << "Unsupported operation.";
  }
}
*/

class PutVisitor : public boost::static_visitor<> {
 public:
  PutVisitor(PmidManagerDispatcher& dispatcher,
             const PmidName& pmid_name,
             const nfs::MessageId& task_id,
             const NonEmptyString& content)
    : dispatcher_(dispatcher),
      kPmidName_(pmid_name),
      kTtaskId_(task_id),
      kContent_(content) {}

  template <typename T>
  void operator()(const T& data_name) {
    dispatcher_.SendPutRequest<T::data_type>(kTtaskId_,
                                             T::data_type(data_name, kContent_),
                                             kPmidName_);
  }

 private:
  PmidManagerDispatcher& dispatcher_;
  const PmidName kPmidName_;
  const nfs::MessageId& kTtaskId_;
  const NonEmptyString kContent_;
};

template<typename T>
void PmidManagerService::HandlePut(
    const T& message,
    const typename T::Sender& /*sender*/,
    const typename T::Receiver& receiver) {
  auto data_name(GetDataNameVariant(message.contents->name.type,
                                    message.contents->name.raw_name));

  PutVisitor put_visitor(dispatcher_,
                         PmidName(Identity(NodeId(receiver).string())),
                         message.message_id,
                         message.contents->content);
  boost::apply_visitor(put_visitor, data_name);
}


//template<typename Data>
//void PmidManagerService::HandlePut(const nfs::Message& message) {
//  try {
//    Data data(typename Data::Name(message.data().name),
//              typename Data::serialised_type(message.data().content));
//    nfs_.Put(PmidName(detail::GetPmidAccountName(message)),
//             data,
//             [this, message](std::string reply) {
//                this->HandlePutCallback<Data>(reply, message);
//             });
//    nfs::Reply reply(maidsafe::CommonErrors::success);
//    accumulator_.SetHandled(message, reply);
//    return;
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
//  SendPutResult<Data>(message, false);
//}


class DeleteVisitor : public boost::static_visitor<> {
 public:
  DeleteVisitor(PmidManagerDispatcher& dispatcher,
             const PmidName& pmid_name,
             const nfs::MessageId& task_id)
    : dispatcher_(dispatcher),
      kPmidName_(pmid_name),
      kTtaskId_(task_id) {}

  template <typename T>
  void operator()(const T& data_name) {
    dispatcher_.SendDeleteRequest<T::data_type>(kTtaskId_, kPmidName_, data_name);
  }

 private:
  PmidManagerDispatcher& dispatcher_;
  const PmidName kPmidName_;
  const nfs::MessageId& kTtaskId_;
};


template<>
void PmidManagerService::HandleMessage(
    const nfs::DeleteRequestFromDataManagerToPmidManager& message,
    const typename nfs::DeleteRequestFromDataManagerToPmidManager::Sender& sender,
    const typename nfs::DeleteRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef nfs::DeleteRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<MessageType,
                          nfs::PmidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>::Value()),
      [this](const MessageType& message,
             const typename MessageType::Sender& sender,
             const typename MessageType::Receiver& receiver) {
        this->HandleDelete(message, sender, receiver);
      },
      accumulator_mutex_)(message, sender, receiver);
}

template<typename T>
void PmidManagerService::HandleDelete(
    const T& message,
    const typename T::Sender& /*sender*/,
    const typename T::Receiver& receiver) {
  auto data_name(GetDataNameVariant(message.contents->type,
                                    message.contents->raw_name));

  DeleteVisitor delete_visitor(dispatcher_,
                               PmidName(Identity(NodeId(receiver).string())),
                               message.message_id);
  boost::apply_visitor(delete_visitor, data_name);
}



//template<typename Data>
//void PmidManagerService::HandleDelete(const nfs::Message& message,
//                                      const routing::ReplyFunctor& /*reply_functor*/) {
//  try {
//    auto account_name(detail::GetPmidAccountName(message));
//    typename Data::Name data_name(message.data().name);
//    try  {
//      pmid_account_handler_.Delete<Data>(account_name, data_name);
//    }
//    catch(const maidsafe_error&) {
//      AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kDelete>(message);
//      return;
//    }
//    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kDelete>(message);
//    nfs_.Delete<Data>(message.pmid_node(), data_name, [](std::string) {});
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
//}

// Commented by Mahmoud on 7 Sep.
//template<typename Data>
//void PmidManagerService::HandlePutCallback(const std::string& serialised_reply,
//                                           const nfs::Message& message) {
//  nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
//  if (reply.IsSuccess()) {
//    pmid_account_handler_.CreateAccount(PmidName(detail::GetPmidAccountName(message)));
//    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message);
//    SendPutResult<Data>(message, true);
//  } else {
//    SendPutResult<Data>(message, false);
//  }
//}

// Commented by Mahmoud on 7 Sep.
//template<typename Data>
//void PmidManagerService::SendPutResult(const nfs::Message& message, bool result) {
//  std::lock_guard<std::mutex> lock(accumulator_mutex_);
//  protobuf::PutResult proto_put_result;

//  proto_put_result.set_result(result);
//  proto_put_result.set_pmid_name(message.pmid_node()->string());
//  if (result) {
//    proto_put_result.set_data_size(message.data().content.string().size());
//  } else {
//    proto_put_result.set_serialised_data(message.Serialise()->string());
//  }
//  nfs_.SendPutResult<Data>(Data::Name(message.data().name),
//                           NonEmptyString(proto_put_result.SerializeAsString()));
//}

// Commented by Mahmoud on 7 Sep.
//template<typename Data, nfs::MessageAction Action>
//void PmidManagerService::AddLocalUnresolvedEntryThenSync(const nfs::Message& message) {
//  auto account_name(detail::GetPmidAccountName(message));
//  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, Action>(message, routing_.kNodeId()));
//  pmid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
//  Sync(account_name);
//}

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_INL_H_
