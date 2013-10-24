/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_OPERATION_HANDLERS_H_
#define MAIDSAFE_VAULT_OPERATION_HANDLERS_H_


#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"


namespace maidsafe {

namespace vault {

class PmidNodeService;
class MaidManagerService;
class DataManagerService;
class VersionHandlerService;

namespace detail {

template <typename T>
struct RequiredValue {};

template <>
struct RequiredValue<routing::SingleSource> {
  int operator()() const { return 1; }
};

template <>
struct RequiredValue<routing::GroupSource> {
  int operator()() const { return routing::Parameters::node_group_size - 1; }
};


template <typename ValidateSender, typename AccumulatorType, typename Checker,
          typename ServiceHandlerType>
struct OperationHandler {
  OperationHandler(ValidateSender validate_sender_in, AccumulatorType& accumulator_in,
                   Checker checker_in, ServiceHandlerType* service_in, std::mutex& mutex_in)
      : validate_sender(validate_sender_in),
        accumulator(accumulator_in),
        checker(checker_in),
        service(service_in),
        mutex(mutex_in) {}

  template <typename MessageType, typename Sender, typename Receiver>
  void operator()(const MessageType& message, const Sender& sender, const Receiver& receiver);

 private:
  ValidateSender validate_sender;
  AccumulatorType& accumulator;
  Checker checker;
  ServiceHandlerType* service;
  std::mutex& mutex;
};

template <typename ValidateSender, typename AccumulatorType, typename Checker,
          typename ServiceHandlerType>
template <typename MessageType, typename Sender, typename Receiver>
void OperationHandler<ValidateSender, AccumulatorType, Checker, ServiceHandlerType>::operator()(
    const MessageType& message, const Sender& sender, const Receiver& receiver) {
  if (!validate_sender(message, sender))
    return;
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (accumulator.CheckHandled(message))
      return;
    if (accumulator.AddPendingRequest(message, sender, checker)
           != AccumulatorType::AddResult::kSuccess)
      return;
  }
  DoOperation<ServiceHandlerType, MessageType>(service, message, sender, receiver);
}

template<>
template<>
void OperationHandler<
         typename ValidateSenderType<GetPmidAccountResponseFromPmidManagerToPmidNode>::type,
         Accumulator<PmidNodeServiceMessages>,
         typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor,
         PmidNodeService>::operator()(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

template <typename ServiceHandlerType, typename MessageType>
void DoOperation(ServiceHandlerType* /*service*/, const MessageType& /*message*/,
                 const typename MessageType::Sender& /*sender*/,
                 const typename MessageType::Receiver& /*receiver*/) {
  //  MessageType::Invalid_function_call;
}

// TODO(Team) Consider moving these to respective persona
//=============================== To MaidManager ===================================================

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/);

template <>
void DoOperation(MaidManagerService* service,
                 const PutResponseFromDataManagerToMaidManager& message,
                 const PutResponseFromDataManagerToMaidManager::Sender& /*sender*/,
                 const PutResponseFromDataManagerToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const PutFailureFromDataManagerToMaidManager& message,
                 const PutFailureFromDataManagerToMaidManager::Sender& sender,
                 const PutFailureFromDataManagerToMaidManager::Receiver& /*receiver*/);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/);

template <>
void DoOperation(MaidManagerService* service,
                 const PmidHealthResponseFromPmidManagerToMaidManager& message,
                 const PmidHealthResponseFromPmidManagerToMaidManager::Sender& sender,
                 const PmidHealthResponseFromPmidManagerToMaidManager::Receiver& receiver);

//=============================== To DataManager ===================================================
template <>
void DoOperation(DataManagerService* service,
                 const PutRequestFromMaidManagerToDataManager& message,
                 const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMaidManagerToDataManager::Receiver&);

template <>
void DoOperation(DataManagerService* service,
                 const PutResponseFromPmidManagerToDataManager& message,
                 const PutResponseFromPmidManagerToDataManager::Sender& sender,
                 const PutResponseFromPmidManagerToDataManager::Receiver& /*receiver*/);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromMaidNodeToDataManager& message,
                 const nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
                 const nfs::GetRequestFromMaidNodeToDataManager::Receiver& /*receiver*/);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterToDataManager& message,
                 const nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterToDataManager::Receiver& /*receiver*/);

template <>
void DoOperation(DataManagerService* service,
                 const GetResponseFromPmidNodeToDataManager& message,
                 const GetResponseFromPmidNodeToDataManager::Sender& sender,
                 const GetResponseFromPmidNodeToDataManager::Receiver& /*receiver*/);

template <>
void DoOperation(DataManagerService* service,
                 const DeleteRequestFromMaidManagerToDataManager& message,
                 const DeleteRequestFromMaidManagerToDataManager::Sender& /*sender*/,
                 const DeleteRequestFromMaidManagerToDataManager::Receiver& /*receiver*/);

template <>
void DoOperation(DataManagerService* service,
                 const PutFailureFromPmidManagerToDataManager& message,
                 const PutFailureFromPmidManagerToDataManager::Sender& sender,
                 const PutFailureFromPmidManagerToDataManager::Receiver& /*receiver*/);

//=============================== To PmidManager ===================================================
template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const PutRequestFromDataManagerToPmidManager& message,
                 const PutRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerPutVisitor<ServiceHandlerType> put_visitor(service, message.contents->content,
                                                        message.message_id, receiver);
  boost::apply_visitor(put_visitor(), data_name);
}

template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const PutFailureFromPmidNodeToPmidManager& message,
                 const PutFailureFromPmidNodeToPmidManager::Sender& /*sender*/,
                 const PutFailureFromPmidNodeToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  PutResponseFailureVisitor<ServiceHandlerType> put_failure_visitor(
      service, MaidName(Identity(receiver.data.string())), message.contents->return_code,
                        message.message_id);
  boost::apply_visitor(put_failure_visitor(), data_name);
}

template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const DeleteRequestFromDataManagerToPmidManager& message,
                 const DeleteRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const DeleteRequestFromDataManagerToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(message.contents));
  PmidManagerDeleteVisitor<ServiceHandlerType> delete_visitor(
      service, PmidName(Identity(receiver.data.string())), message.message_id);
  boost::apply_visitor(delete_visitor(), data_name);
}

template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager& message,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager::Sender& sender,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager::Receiver& /*receiver*/) {
  service->HandleSendPmidAccount(PmidName(Identity(sender.data.string())),  *message.contents);
}

template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const PmidHealthRequestFromMaidNodeToPmidManager& message,
                 const PmidHealthRequestFromMaidNodeToPmidManager::Sender& sender,
                 const PmidHealthRequestFromMaidNodeToPmidManager::Receiver& receiver) {
  service->HandleHealthRequest(PmidName(Identity(receiver.data.string())),
                               MaidName(Identity(sender.data.string())),
                               message.message_id);
}

//=============================== To PmidNode ======================================================
template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const DeleteRequestFromPmidManagerToPmidNode& message,
                 const DeleteRequestFromPmidManagerToPmidNode::Sender& /*sender*/,
                 const DeleteRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(message.contents));
  PmidNodeDeleteVisitor<ServiceHandlerType> delete_visitor(service);
  boost::apply_visitor(delete_visitor(), data_name);
}

template <typename ServiceHandlerType>
void DoOperation(ServiceHandlerType* service,
                 const PutRequestFromPmidManagerToPmidNode& message,
                 const PutRequestFromPmidManagerToPmidNode::Sender& /*sender*/,
                 const PutRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  PmidNodePutVisitor<ServiceHandlerType> put_visitor(service, message.contents->content,
                                                     message.message_id);
  boost::apply_visitor(put_visitor(), data_name);
}

//====================================== To VersionHandler =========================================

template<>
void DoOperation(VersionHandlerService* service,
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void DoOperation(VersionHandlerService* service,
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void DoOperation(VersionHandlerService* service,
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& receiver);

template<>
void DoOperation(VersionHandlerService* service,
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& receiver);

}  // namespace detail

template <typename ServiceHandler, typename MessageType>
struct OperationHandlerWrapper {
  typedef Accumulator<typename ServiceHandler::Messages> AccumulatorType;
  typedef detail::OperationHandler<typename detail::ValidateSenderType<MessageType>::type,
                                   AccumulatorType,
                                   typename AccumulatorType::AddCheckerFunctor,
                                   ServiceHandler> TypedOperationHandler;

  OperationHandlerWrapper(AccumulatorType& accumulator,
                          typename detail::ValidateSenderType<MessageType>::type validate_sender,
                          typename AccumulatorType::AddCheckerFunctor checker,
                          ServiceHandler* service, std::mutex& mutex)
      : typed_operation_handler(validate_sender, accumulator, checker, service, mutex) {}

  void operator()(const MessageType& message, const typename MessageType::Sender& sender,
                  const typename MessageType::Receiver& receiver) {
    typed_operation_handler(message, sender, receiver);
  }

 private:
  TypedOperationHandler typed_operation_handler;
};

template <typename Message>
int RequiredRequests(const Message&) {
  return detail::RequiredValue<typename Message::Sender>()();
}

}  // namespace vault

}  // namespace maidsafe


#endif // MAIDSAFE_VAULT_OPERATION_HANDLERS_H_
