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


#include "boost/variant.hpp"

#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/pmid_node/service.h"

namespace maidsafe {

namespace vault {

namespace detail {

//=============================== To MaidManager ===================================================

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
                 const nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  service->HandleCreateMaidAccount(message.contents->public_maid(),
                                   message.contents->public_anmaid(),
                                   message.message_id);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
                 const nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  service->HandlePmidRegistration(nfs_vault::PmidRegistration(message.contents->Serialise()));
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
std::cout << "put visitor" << std::endl;
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutVisitor<MaidManagerService> put_visitor(service, message.contents->data.content,
                                                        sender.data, message.contents->pmid_hint,
                                                        message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const PutResponseFromDataManagerToMaidManager& message,
                 const PutResponseFromDataManagerToMaidManager::Sender& /*sender*/,
                 const PutResponseFromDataManagerToMaidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutResponseVisitor<MaidManagerService> put_response_visitor(
      service, Identity(receiver.data.string()),
      message.contents->cost, message.message_id);
  boost::apply_visitor(put_response_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const PutFailureFromDataManagerToMaidManager& message,
                 const PutFailureFromDataManagerToMaidManager::Sender& sender,
                 const PutFailureFromDataManagerToMaidManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutResponseFailureVisitor<MaidManagerService> put_visitor(
      service, MaidName(Identity(sender.sender_id.data.string())),
      message.contents->return_code.value, message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerDeleteVisitor<MaidManagerService> delete_visitor(
      service, MaidName(Identity(sender.data.string())), message.message_id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const PmidHealthResponseFromPmidManagerToMaidManager& message,
                 const PmidHealthResponseFromPmidManagerToMaidManager::Sender& sender,
                 const PmidHealthResponseFromPmidManagerToMaidManager::Receiver& receiver) {
  service->HandleHealthResponse(MaidName(Identity(receiver.data.string())),
                                PmidName(Identity(sender.group_id.data.string())),
                                message.contents->pmid_health.serialised_pmid_health,
                                message.contents->return_code,
                                message.message_id);
}

//=============================== To DataManager ===================================================

template <>
void DoOperation(DataManagerService* service,
                 const PutRequestFromMaidManagerToDataManager& message,
                 const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMaidManagerToDataManager::Receiver&) {
  auto data_name(GetNameVariant(*message.contents));
  DataManagerPutVisitor<DataManagerService> put_visitor(
      service, message.contents->data.content, Identity(sender.group_id.data.string()),
      message.contents->pmid_hint, message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const PutResponseFromPmidManagerToDataManager& message,
                 const PutResponseFromPmidManagerToDataManager::Sender& sender,
                 const PutResponseFromPmidManagerToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  DataManagerPutResponseVisitor<DataManagerService> put_response_visitor(
      service, PmidName(Identity(sender.group_id.data.string())), message.contents->size,
      message.message_id);
  boost::apply_visitor(put_response_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromMaidNodeToDataManager& message,
                 const nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
                 const nfs::GetRequestFromMaidNodeToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromMaidNodeToDataManager::SourcePersona SourceType;
  Requestor<SourceType> requestor(sender.data);
  GetRequestVisitor<DataManagerService, Requestor<SourceType>> get_request_visitor(
      service, requestor, message.message_id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterToDataManager& message,
                 const nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromDataGetterToDataManager::SourcePersona SourceType;
  Requestor<SourceType> requestor(sender.data);
  GetRequestVisitor<DataManagerService, Requestor<SourceType>> get_request_visitor(
          service, requestor, message.message_id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const GetResponseFromPmidNodeToDataManager& message,
                 const GetResponseFromPmidNodeToDataManager::Sender& sender,
                 const GetResponseFromPmidNodeToDataManager::Receiver& /*receiver*/) {
  service->HandleGetResponse(PmidName(Identity(sender.data.string())),
                             message.message_id, *message.contents);
}

template <>
void DoOperation(DataManagerService* service,
                 const DeleteRequestFromMaidManagerToDataManager& message,
                 const DeleteRequestFromMaidManagerToDataManager::Sender& /*sender*/,
                 const DeleteRequestFromMaidManagerToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  DataManagerDeleteVisitor<DataManagerService> delete_visitor(service, message.message_id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const PutFailureFromPmidManagerToDataManager& message,
                 const PutFailureFromPmidManagerToDataManager::Sender& sender,
                 const PutFailureFromPmidManagerToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  PutResponseFailureVisitor<DataManagerService> put_visitor(
      service, Identity(sender.sender_id.data.string()),
      message.contents->return_code.value, message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

//=============================== To PmidManager ===================================================

template <>
void DoOperation(PmidManagerService* service,
                 const PutRequestFromDataManagerToPmidManager& message,
                 const PutRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerPutVisitor<PmidManagerService> put_visitor(service, message.contents->content,
                                                        Identity(receiver.data.string()),
                                                        message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service,
                 const PutFailureFromPmidNodeToPmidManager& message,
                 const PutFailureFromPmidNodeToPmidManager::Sender& /*sender*/,
                 const PutFailureFromPmidNodeToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerPutResponseFailureVisitor<PmidManagerService> put_failure_visitor(
      service, PmidName(Identity(receiver.data.string())),
      message.contents->available_space, message.contents->return_code.value, message.message_id);
  boost::apply_visitor(put_failure_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service,
                 const DeleteRequestFromDataManagerToPmidManager& message,
                 const DeleteRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const DeleteRequestFromDataManagerToPmidManager::Receiver& receiver) {
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerDeleteVisitor<PmidManagerService> delete_visitor(
      service, PmidName(Identity(receiver.data.string())), message.message_id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager& message,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager::Sender& sender,
                 const GetPmidAccountRequestFromPmidNodeToPmidManager::Receiver& /*receiver*/) {
  service->HandleSendPmidAccount(PmidName(Identity(sender.data.string())),
                                 message.contents->available_size);
}

template <>
void DoOperation(PmidManagerService* service,
                 const PmidHealthRequestFromMaidNodeToPmidManager& message,
                 const PmidHealthRequestFromMaidNodeToPmidManager::Sender& sender,
                 const PmidHealthRequestFromMaidNodeToPmidManager::Receiver& receiver) {
  service->HandleHealthRequest(PmidName(Identity(receiver.data.string())),
                               MaidName(Identity(sender.data.string())),
                               message.message_id);
}

//=============================== To PmidNode ======================================================

template <>
void DoOperation(PmidNodeService* service,
                 const DeleteRequestFromPmidManagerToPmidNode& message,
                 const DeleteRequestFromPmidManagerToPmidNode::Sender& /*sender*/,
                 const DeleteRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  PmidNodeDeleteVisitor<PmidNodeService> delete_visitor(service);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(PmidNodeService* service,
                 const PutRequestFromPmidManagerToPmidNode& message,
                 const PutRequestFromPmidManagerToPmidNode::Sender& /*sender*/,
                 const PutRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  PmidNodePutVisitor<PmidNodeService> put_visitor(service, message.contents->content,
                                                     message.message_id);
  boost::apply_visitor(put_visitor, data_name);
}

//====================================== To VersionHandler =========================================

template<>
void DoOperation(VersionHandlerService* /*service*/,
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& /*message*/,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& /*sender*/,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& /*receiver*/) {}

template<>
void DoOperation(VersionHandlerService* /*service*/,
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& /*message*/,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& /*sender*/,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& /*receiver*/) {}

template<>
void DoOperation(VersionHandlerService* /*service*/,
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& /*message*/,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& /*sender*/,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& /*receiver*/) {}

template<>
void DoOperation(VersionHandlerService* /*service*/,
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& /*message*/,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& /*sender*/,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& /*receiver*/) {}


// ================================================================================================


template <>
template <>
void OperationHandler<
         typename ValidateSenderType<GetPmidAccountResponseFromPmidManagerToPmidNode>::type,
         Accumulator<PmidNodeServiceMessages>,
         typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor,
         PmidNodeService>::operator()(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  if (!validate_sender(message, sender))
    return;
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (accumulator.CheckHandled(message))
      return;
    auto result(accumulator.AddPendingRequest(message, sender, checker));
    if (result == Accumulator<PmidNodeServiceMessages>::AddResult::kSuccess) {
      int failures(0);
      auto responses(accumulator.Get(message));
      std::vector<std::set<nfs_vault::DataName>> response_vec;
      for (const auto& response : responses) {
        auto typed_response(boost::get<GetPmidAccountResponseFromPmidManagerToPmidNode>(response));
        if (typed_response.contents->return_code.value.code() == CommonErrors::success)
          response_vec.push_back(typed_response.contents->names);
        else
         failures++;
      }
      service->HandlePmidAccountResponses(response_vec, failures);
    } else if (result == Accumulator<PmidNodeServiceMessages>::AddResult::kFailure) {
      service->StartUp();
    }
  }
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
