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

template <typename>
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
