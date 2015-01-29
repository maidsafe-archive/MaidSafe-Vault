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
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/mpid_manager/service.h"

namespace maidsafe {

namespace vault {

class PmidNodeService;
class MaidManagerService;
class DataManagerService;
class PmidManagerService;
class PmidNodeService;
class VersionHandlerService;
class MpidManagerService;

namespace detail {

template <typename T>
struct RequiredValue {};

template <>
struct RequiredValue<routing::SingleSource> {
  int operator()() const { return 1; }
};

template <>
struct RequiredValue<routing::SingleRelaySource> {
  int operator()() const { return 1; }
};

template <>
struct RequiredValue<routing::GroupSource> {
  int operator()() const { return routing::Parameters::group_size - 1; }
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
//   LOG(kVerbose) << "OperationHandler::operator()";
  if (!validate_sender(message, sender)) {
    LOG(kError) << "invalid sender";
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (accumulator.AddPendingRequest(message, sender, checker) !=
        AccumulatorType::AddResult::kSuccess) {
      LOG(kInfo) << "AddPendingRequest unsuccessful";
      return;
    }
  }
  DoOperation<ServiceHandlerType, MessageType>(service, message, sender, receiver);
}

template <>
template <>
void OperationHandler<typename ValidateSenderType<PutRequestFromDataManagerToPmidManager>::type,
                      Accumulator<PmidManagerServiceMessages>,
                      typename Accumulator<PmidManagerServiceMessages>::AddCheckerFunctor,
                      PmidManagerService>::
    operator()(const PutRequestFromDataManagerToPmidManager& message,
               const PutRequestFromDataManagerToPmidManager::Sender& sender,
               const PutRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
template <>
void OperationHandler<typename ValidateSenderType<GetRequestFromDataManagerToPmidNode>::type,
                      Accumulator<PmidNodeServiceMessages>,
                      typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor,
                      PmidNodeService>::
    operator()(const GetRequestFromDataManagerToPmidNode& message,
               const GetRequestFromDataManagerToPmidNode::Sender& sender,
               const GetRequestFromDataManagerToPmidNode::Receiver& receiver);

template <>
template <>
void OperationHandler<
    typename ValidateSenderType<IntegrityCheckRequestFromDataManagerToPmidNode>::type,
    Accumulator<PmidNodeServiceMessages>,
    typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor, PmidNodeService>::
    operator()(const IntegrityCheckRequestFromDataManagerToPmidNode& message,
               const IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
               const IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver);

template <typename ServiceHandlerType, typename MessageType>
void DoOperation(ServiceHandlerType* /*service*/, const MessageType& /*message*/,
                 const typename MessageType::Sender& /*sender*/,
                 const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_generic_handler_is_available__Specialisation_is_required;
}

// TODO(Team) Consider moving these to respective persona
//=============================== To MaidManager ===================================================

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const PutResponseFromDataManagerToMaidManager& message,
                 const PutResponseFromDataManagerToMaidManager::Sender& sender,
                 const PutResponseFromDataManagerToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service, const PutFailureFromDataManagerToMaidManager& message,
                 const PutFailureFromDataManagerToMaidManager::Sender& sender,
                 const PutFailureFromDataManagerToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(
    MaidManagerService* service,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const PutVersionResponseFromVersionHandlerToMaidManager& message,
                 const PutVersionResponseFromVersionHandlerToMaidManager::Sender& sender,
                 const PutVersionResponseFromVersionHandlerToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager& message,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void DoOperation(MaidManagerService* service,
                 const CreateVersionTreeResponseFromVersionHandlerToMaidManager& message,
                 const CreateVersionTreeResponseFromVersionHandlerToMaidManager::Sender& sender,
                 const CreateVersionTreeResponseFromVersionHandlerToMaidManager::Receiver&);

template <>
void DoOperation(MaidManagerService* service,
                 const AccountQueryFromMaidManagerToMaidManager& message,
                 const AccountQueryFromMaidManagerToMaidManager::Sender& sender,
                 const AccountQueryFromMaidManagerToMaidManager::Receiver& receiver);

//=============================== To DataManager ===================================================
template <>
void DoOperation(DataManagerService* service, const PutRequestFromMaidManagerToDataManager& message,
                 const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMaidManagerToDataManager::Receiver&);
template <>
void DoOperation(DataManagerService* service, const PutRequestFromMpidManagerToDataManager& message,
                 const typename PutRequestFromMpidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMpidManagerToDataManager::Receiver&);

template <>
void DoOperation(DataManagerService* service,
                 const PutResponseFromPmidManagerToDataManager& message,
                 const PutResponseFromPmidManagerToDataManager::Sender& sender,
                 const PutResponseFromPmidManagerToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromMpidNodeToDataManager& message,
                 const nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
                 const nfs::GetRequestFromMpidNodeToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromMpidNodeToDataManager& message,
                 const nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
                 const nfs::GetRequestFromMpidNodeToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterToDataManager& message,
                 const nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterPartialToDataManager& message,
                 const nfs::GetRequestFromDataGetterPartialToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterPartialToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service, const GetResponseFromPmidNodeToDataManager& message,
                 const GetResponseFromPmidNodeToDataManager::Sender& sender,
                 const GetResponseFromPmidNodeToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const DeleteRequestFromMaidManagerToDataManager& message,
                 const DeleteRequestFromMaidManagerToDataManager::Sender& sender,
                 const DeleteRequestFromMaidManagerToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service, const PutFailureFromPmidManagerToDataManager& message,
                 const PutFailureFromPmidManagerToDataManager::Sender& sender,
                 const PutFailureFromPmidManagerToDataManager::Receiver& receiver);

template <>
void DoOperation(DataManagerService* service,
                 const AccountQueryFromDataManagerToDataManager& message,
                 const AccountQueryFromDataManagerToDataManager::Sender& sender,
                 const AccountQueryFromDataManagerToDataManager::Receiver& receiver);

//=============================== To PmidManager ===================================================
template <>
void DoOperation(PmidManagerService* service, const PutRequestFromDataManagerToPmidManager& message,
                 const PutRequestFromDataManagerToPmidManager::Sender& sender,
                 const PutRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void DoOperation(PmidManagerService* service, const PutFailureFromPmidNodeToPmidManager& message,
                 const PutFailureFromPmidNodeToPmidManager::Sender& sender,
                 const PutFailureFromPmidNodeToPmidManager::Receiver& receiver);

template <>
void DoOperation(PmidManagerService* service,
                 const DeleteRequestFromDataManagerToPmidManager& message,
                 const DeleteRequestFromDataManagerToPmidManager::Sender& sender,
                 const DeleteRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void DoOperation(PmidManagerService* service,
                 const IntegrityCheckRequestFromDataManagerToPmidManager& message,
                 const IntegrityCheckRequestFromDataManagerToPmidManager::Sender& sender,
                 const IntegrityCheckRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void DoOperation(PmidManagerService* service,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager& message,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager::Sender& sender,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);

template <>
void DoOperation(PmidManagerService* service,
                 const UpdateAccountFromDataManagerToPmidManager& message,
                 const UpdateAccountFromDataManagerToPmidManager::Sender& sender,
                 const UpdateAccountFromDataManagerToPmidManager::Receiver& receiver);

//=============================== To PmidNode ======================================================
template <>
void DoOperation(PmidNodeService* service, const DeleteRequestFromPmidManagerToPmidNode& message,
                 const DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
                 const DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver);

template <>
void DoOperation(PmidNodeService* service, const PutRequestFromPmidManagerToPmidNode& message,
                 const PutRequestFromPmidManagerToPmidNode::Sender& sender,
                 const PutRequestFromPmidManagerToPmidNode::Receiver& receiver);

template <>
void DoOperation(PmidNodeService* service, const GetRequestFromDataManagerToPmidNode& message,
                 const GetRequestFromDataManagerToPmidNode::Sender& sender,
                 const GetRequestFromDataManagerToPmidNode::Receiver& receiver);
template <>
void DoOperation(PmidNodeService* service,
                 const IntegrityCheckRequestFromDataManagerToPmidNode& message,
                 const IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
                 const IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver);

//====================================== To VersionHandler =========================================

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service, const PutVersionRequestFromMaidManagerToVersionHandler& message,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service,
    const DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler& message,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Receiver& receiver);

template <>
void DoOperation(
    VersionHandlerService* service,
    const CreateVersionTreeRequestFromMaidManagerToVersionHandler& message,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Receiver& receiver);

//====================================== To MpidManager ===========================================

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::CreateAccountRequestFromMpidNodeToMpidManager& message,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const SendAlertFromMpidManagerToMpidManager& message,
    const typename SendAlertFromMpidManagerToMpidManager::Sender& sender,
    const typename SendAlertFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::GetMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const GetRequestFromMpidManagerToMpidManager& message,
    const typename GetRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename GetRequestFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const GetResponseFromMpidManagerToMpidManager& message,
    const typename GetResponseFromMpidManagerToMpidManager::Sender& sender,
    const typename GetResponseFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::DeleteRequestFromMpidNodeToMpidManager& message,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::SendMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const DeleteRequestFromMpidManagerToMpidManager& message,
    const typename DeleteRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename DeleteRequestFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void DoOperation(
    MpidManagerService* service,
    const PutResponseFromDataManagerToMpidManager& message,
    const typename PutResponseFromDataManagerToMpidManager::Sender& sender,
    const typename PutResponseFromDataManagerToMpidManager::Receiver& receiver);

}  // namespace detail

template <typename ServiceHandler, typename MessageType,
          typename AccumulatorT = Accumulator<typename ServiceHandler::Messages>>
struct OperationHandlerWrapper {
  typedef AccumulatorT AccumulatorType;
  typedef detail::OperationHandler<typename detail::ValidateSenderType<MessageType>::type,
                                   AccumulatorType, typename AccumulatorType::AddCheckerFunctor,
                                   ServiceHandler> TypedOperationHandler;

  OperationHandlerWrapper(AccumulatorType& accumulator,
                          typename detail::ValidateSenderType<MessageType>::type validate_sender,
                          typename AccumulatorType::AddCheckerFunctor checker,
                          ServiceHandler* service, std::mutex& mutex)
      : typed_operation_handler(validate_sender, accumulator, checker, service, mutex) {}

  void operator()(const MessageType& message, const typename MessageType::Sender& sender,
                  const typename MessageType::Receiver& receiver) {
    LOG(kVerbose) << message;
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


#endif  // MAIDSAFE_VAULT_OPERATION_HANDLERS_H_
