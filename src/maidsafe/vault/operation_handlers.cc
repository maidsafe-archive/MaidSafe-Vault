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

#include <set>
#include <string>
#include <vector>

#include "boost/variant.hpp"

#include "maidsafe/nfs/vault/messages.h"

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
  LOG(kVerbose) << "DoOperation CreateAccountRequestFromMaidNodeToMaidManager" << message.id;
  service->HandleCreateMaidAccount(message.contents->public_maid(),
                                   message.contents->public_anmaid(), message.id);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation PutRequestFromMaidNodeToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutVisitor<MaidManagerService> put_visitor(service, message.contents->content,
                                                        sender.data, message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const PutResponseFromDataManagerToMaidManager& message,
                 const PutResponseFromDataManagerToMaidManager::Sender& /*sender*/,
                 const PutResponseFromDataManagerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutResponseFromDataManagerToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  PutResponseVisitor<MaidManagerService, MaidName> put_response_visitor(
      service, Identity(receiver.data.string()), message.contents->cost, message.id);
  boost::apply_visitor(put_response_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service, const PutFailureFromDataManagerToMaidManager& message,
                 const PutFailureFromDataManagerToMaidManager::Sender& /*sender*/,
                 const PutFailureFromDataManagerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutFailureFromDataManagerToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutResponseFailureVisitor<MaidManagerService> put_visitor(
      service, MaidName(Identity(receiver.data.string())), message.contents->return_code.value,
      message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation DeleteRequestFromMaidNodeToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerDeleteVisitor<MaidManagerService> delete_visitor(
      service, MaidName(Identity(sender.data.string())), message.id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation PutVersionRequestFromMaidNodeToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerPutVersionRequestVisitor<MaidManagerService> put_version_visitor(
      service, MaidName(Identity(sender.data.string())), message.contents->old_version_name,
      message.contents->new_version_name, message.id);
  boost::apply_visitor(put_version_visitor, data_name);
}

template <>
void DoOperation(
    MaidManagerService* service,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation DeleteBranchUntilForkRequestFromMaidNodeToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerDeleteBranchUntilForkVisitor<MaidManagerService> delete_version_visitor(
      service, MaidName(Identity(sender.data.string())), message.contents->version_name,
      message.id);
  boost::apply_visitor(delete_version_visitor, data_name);
}

template <>
void DoOperation(MaidManagerService* /* service*/,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& /*message*/,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& /*sender*/,
                 const nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& /*receiver*/) {
  //  service->HandleRemoveAccount(MaidName(Identity(sender.data.string())));
}

template <>
void DoOperation(MaidManagerService* service,
                 const PutVersionResponseFromVersionHandlerToMaidManager& message,
                 const PutVersionResponseFromVersionHandlerToMaidManager::Sender& /*sender*/,
                 const PutVersionResponseFromVersionHandlerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutVersionResponseFromVersionHandlerToMaidManager";
  std::unique_ptr<StructuredDataVersions::VersionName> tip_of_tree;
  if (message.contents->tip_of_tree)
    tip_of_tree.reset(new StructuredDataVersions::VersionName(*message.contents->tip_of_tree));
  service->HandlePutVersionResponse(MaidName(Identity(receiver.data.string())),
                                    message.contents->return_code.value, std::move(tip_of_tree),
                                    message.id);
}

template <>
void DoOperation(MaidManagerService* service,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager& message,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Sender& sender,
                 const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Receiver&) {
  LOG(kVerbose) << "DoOperation CreateVersionTreeRequestFromMaidNodeToMaidManager";
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerCreateVersionTreeRequestVisitor<MaidManagerService> create_version_tree_visitor(
      service, MaidName(Identity(sender.data.string())), message.contents->version_name,
      message.contents->max_versions, message.contents->max_branches, message.id);
  boost::apply_visitor(create_version_tree_visitor, data_name);
}

template <>
void DoOperation(
    MaidManagerService* service,
    const CreateVersionTreeResponseFromVersionHandlerToMaidManager& message,
    const CreateVersionTreeResponseFromVersionHandlerToMaidManager::Sender& /*sender*/,
    const CreateVersionTreeResponseFromVersionHandlerToMaidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation CreateVersionTreeResponseFromVersionHandlerToMaidManager";
  service->HandleCreateVersionTreeResponse(MaidName(Identity(receiver.data.string())),
                                           message.contents->value, message.id);
}

template <>
void DoOperation(MaidManagerService* service,
                 const AccountQueryFromMaidManagerToMaidManager& message,
                 const AccountQueryFromMaidManagerToMaidManager::Sender& sender,
                 const AccountQueryFromMaidManagerToMaidManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  MaidManagerAccountRequestVisitor<MaidManagerService> account_request_visitor(service, sender);
  boost::apply_visitor(account_request_visitor, data_name);
}

//=============================== To DataManager ===================================================

template <>
void DoOperation(DataManagerService* service, const PutRequestFromMaidManagerToDataManager& message,
                 const typename PutRequestFromMaidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMaidManagerToDataManager::Receiver&) {
  LOG(kVerbose) << "DoOperation PutRequestFromMaidManagerToDataManager";
  using  SourceType = PutRequestFromMaidManagerToDataManager::SourcePersona;
  Requestor<SourceType> requestor(sender.group_id.data);
  auto data_name(GetNameVariant(*message.contents));
  DataManagerPutVisitor<DataManagerService, Requestor<SourceType>>
      put_visitor(service, message.contents->content, requestor, message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service, const PutRequestFromMpidManagerToDataManager& message,
                 const typename PutRequestFromMpidManagerToDataManager::Sender& sender,
                 const typename PutRequestFromMpidManagerToDataManager::Receiver&) {
  LOG(kVerbose) << "DoOperation PutRequestFromMpidManagerToDataManager";
  using  SourceType = PutRequestFromMpidManagerToDataManager::SourcePersona;
  Requestor<SourceType> requestor(sender.group_id.data);
  auto data_name(GetNameVariant(*message.contents));
  DataManagerPutVisitor<DataManagerService, Requestor<SourceType>>
  put_visitor(service, message.contents->content, requestor, message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const PutResponseFromPmidManagerToDataManager& message,
                 const PutResponseFromPmidManagerToDataManager::Sender& sender,
                 const PutResponseFromPmidManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation PutResponseFromPmidManagerToDataManager received from sender "
                << HexSubstr(sender.sender_id.data.string()) << " regarding the group of "
                << HexSubstr(sender.group_id.data.string()) << " msg id: " << message.id;
  auto data_name(GetNameVariant(*message.contents));
  DataManagerPutResponseVisitor<DataManagerService> put_response_visitor(
      service, PmidName(Identity(sender.group_id.data.string())), message.id);
  boost::apply_visitor(put_response_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromMpidNodeToDataManager& message,
                 const nfs::GetRequestFromMpidNodeToDataManager::Sender& sender,
                 const nfs::GetRequestFromMpidNodeToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromMpidNodeToDataManager::SourcePersona SourceType;
  Requestor<SourceType> requestor(sender.data);
  GetRequestVisitor<DataManagerService, Requestor<SourceType>> get_request_visitor(
      service, requestor, message.id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterToDataManager& message,
                 const nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetRequestFromDataGetterToDataManager";
  auto data_name(GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromDataGetterToDataManager::SourcePersona SourceType;
  Requestor<SourceType> requestor(sender.data);
  GetRequestVisitor<DataManagerService, Requestor<SourceType>> get_request_visitor(
      service, requestor, message.id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const nfs::GetRequestFromDataGetterPartialToDataManager& message,
                 const nfs::GetRequestFromDataGetterPartialToDataManager::Sender& sender,
                 const nfs::GetRequestFromDataGetterPartialToDataManager::Receiver& /*receiver*/) {
  auto data_name(detail::GetNameVariant(*message.contents));
  typedef nfs::GetRequestFromDataGetterPartialToDataManager::SourcePersona SourceType;
  detail::PartialRequestor<SourceType> requestor(sender);
  detail::GetRequestVisitor<DataManagerService, detail::PartialRequestor<SourceType>>
        get_request_visitor(service, requestor, message.id);
  boost::apply_visitor(get_request_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service, const GetResponseFromPmidNodeToDataManager& message,
                 const GetResponseFromPmidNodeToDataManager::Sender& sender,
                 const GetResponseFromPmidNodeToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetResponseFromPmidNodeToDataManager";
  service->HandleGetResponse(PmidName(Identity(sender.data.string())), message.id,
                             *message.contents);
  LOG(kVerbose) << "Done Operation GetResponseFromPmidNodeToDataManager";
}

template <>
void DoOperation(DataManagerService* service,
                 const DeleteRequestFromMaidManagerToDataManager& message,
                 const DeleteRequestFromMaidManagerToDataManager::Sender& /*sender*/,
                 const DeleteRequestFromMaidManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation DeleteRequestFromMaidManagerToDataManager";
  auto data_name(GetNameVariant(*message.contents));
  DataManagerDeleteVisitor<DataManagerService> delete_visitor(service, message.id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service, const PutFailureFromPmidManagerToDataManager& message,
                 const PutFailureFromPmidManagerToDataManager::Sender& sender,
                 const PutFailureFromPmidManagerToDataManager::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation PutFailureFromPmidManagerToDataManager";
  auto data_name(GetNameVariant(*message.contents));
  PutResponseFailureVisitor<DataManagerService> put_visitor(
      service, message.contents->size, Identity(sender.sender_id.data.string()),
      message.contents->return_code.value, message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(DataManagerService* service,
                 const AccountQueryFromDataManagerToDataManager& message,
                 const AccountQueryFromDataManagerToDataManager::Sender& sender,
                 const AccountQueryFromDataManagerToDataManager::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  DataManagerAccountQueryVisitor<DataManagerService> account_request_visitor(service, sender);
  boost::apply_visitor(account_request_visitor, data_name);
}

//=============================== To PmidManager ===================================================

template <>
void DoOperation(PmidManagerService* service, const PutRequestFromDataManagerToPmidManager& message,
                 const PutRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutRequestFromDataManagerToPmidManager "
                << " message shall go to " << HexSubstr(receiver->string());
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerPutVisitor<PmidManagerService> put_visitor(
      service, message.contents->content, Identity(receiver.data.string()), message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service, const PutFailureFromPmidNodeToPmidManager& message,
                 const PutFailureFromPmidNodeToPmidManager::Sender& /*sender*/,
                 const PutFailureFromPmidNodeToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutFailureFromPmidNodeToPmidManager";
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerPutResponseFailureVisitor<PmidManagerService> put_failure_visitor(
      service, PmidName(Identity(receiver.data.string())), message.contents->size,
      message.contents->available_space, message.contents->return_code.value, message.id);
  boost::apply_visitor(put_failure_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service,
                 const DeleteRequestFromDataManagerToPmidManager& message,
                 const DeleteRequestFromDataManagerToPmidManager::Sender& /*sender*/,
                 const DeleteRequestFromDataManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation DeleteRequestFromDataManagerToPmidManager";
  auto data_name(GetNameVariant(*message.contents));
  PmidManagerDeleteVisitor<PmidManagerService> delete_visitor(
      service, PmidName(Identity(receiver.data.string())), message.contents->size, message.id);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(PmidManagerService* service,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager& message,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager::Sender& sender,
                 const CreatePmidAccountRequestFromMaidManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation CreatePmidAccountRequestFromMaidManagerToPmidManager";
  service->HandleCreatePmidAccountRequest(PmidName(Identity(receiver.data.string())),
                                          MaidName(Identity(sender.group_id->string())),
                                          message.id);
}

template <>
void DoOperation(PmidManagerService* service,
                 const UpdateAccountFromDataManagerToPmidManager& message,
                 const UpdateAccountFromDataManagerToPmidManager::Sender& /*sender*/,
                 const UpdateAccountFromDataManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation UpdateAccountFromDataManagerToPmidManager";
  service->HandleUpdateAccount(PmidName(Identity(receiver.data.string())),
                               message.contents->diff_size);
}

template <>
void DoOperation(PmidManagerService* service,
                 const IntegrityCheckRequestFromDataManagerToPmidManager& message,
                 const IntegrityCheckRequestFromDataManagerToPmidManager::Sender& sender,
                 const IntegrityCheckRequestFromDataManagerToPmidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation IntegrityCheckRequestFromDataManagerToPmidManager from "
                << HexSubstr(sender.sender_id.data.string()) << " for chunk "
                << HexSubstr(message.contents->name.raw_name.string()) << " on pmid_node "
                << HexSubstr(receiver.data.string());
  auto data_name(GetNameVariant(message.contents->name));
  // TODO(TEAM): shall FalseNotification be fired after failing of IntegrityCheck ?
  PmidManagerFalseNotificationVisitor<PmidManagerService> false_notification_visitor(
      service, PmidName(Identity(receiver.data.string())), message.contents->size, message.id);
  boost::apply_visitor(false_notification_visitor, data_name);
}

//=============================== To PmidNode ======================================================

template <>
void DoOperation(PmidNodeService* service, const DeleteRequestFromPmidManagerToPmidNode& message,
                 const DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
                 const DeleteRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation DeleteRequestFromPmidManagerToPmidNode from "
                << HexSubstr(sender.sender_id.data.string()) << " for chunk "
                << HexSubstr(message.contents->raw_name.string());
  auto data_name(GetNameVariant(*message.contents));
  PmidNodeDeleteVisitor<PmidNodeService> delete_visitor(service);
  boost::apply_visitor(delete_visitor, data_name);
}

template <>
void DoOperation(PmidNodeService* service, const PutRequestFromPmidManagerToPmidNode& message,
                 const PutRequestFromPmidManagerToPmidNode::Sender& sender,
                 const PutRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  auto data_name(GetNameVariant(*message.contents));
  LOG(kVerbose) << "DoOperation PutRequestFromPmidManagerToPmidNode from "
                << HexSubstr(sender.sender_id.data.string()) << " for chunk "
                << HexSubstr(message.contents->name.raw_name.string());
  PmidNodePutVisitor<PmidNodeService> put_visitor(service, message.contents->content, message.id);
  boost::apply_visitor(put_visitor, data_name);
}

template <>
void DoOperation(PmidNodeService* service, const GetRequestFromDataManagerToPmidNode& message,
                 const GetRequestFromDataManagerToPmidNode::Sender& sender,
                 const GetRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetRequestFromDataManagerToPmidNode"
                << " from " << HexSubstr(sender.sender_id.data.string()) << " for chunk "
                << HexSubstr(message.contents->raw_name.string());
  auto data_name(GetNameVariant(*message.contents));
  PmidNodeGetVisitor<PmidNodeService> get_visitor(service, sender.sender_id, message.id);
  boost::apply_visitor(get_visitor, data_name);
}

template <>
void DoOperation(PmidNodeService* service,
                 const IntegrityCheckRequestFromDataManagerToPmidNode& message,
                 const IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
                 const IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation IntegrityCheckRequestFromDataManagerToPmidNode";
  auto data_name(GetNameVariant(*message.contents));
  PmidNodeIntegrityCheckVisitor<PmidNodeService> integrity_check_visitor(
      service, message.contents->random_string, sender, message.id);
  boost::apply_visitor(integrity_check_visitor, data_name);
}

//====================================== To VersionHandler =========================================

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetVersionsRequestFromMaidNodeToVersionHandler";
  typedef nfs::GetVersionsRequestFromMaidNodeToVersionHandler MessageType;
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerGetVisitor<MessageType::SourcePersona> get_version_visitor(
      service, Identity(sender.data.string()), message.id);
  boost::apply_visitor(get_version_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetBranchRequestFromMaidNodeToVersionHandler";
  typedef nfs::GetBranchRequestFromMaidNodeToVersionHandler MessageType;
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerGetBranchVisitor<MessageType::SourcePersona> get_branch_visitor(
      service, message.contents->version_name, Identity(sender.data.string()), message.id);
  boost::apply_visitor(get_branch_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetVersionsRequestFromDataGetterToVersionHandler";
  typedef nfs::GetVersionsRequestFromDataGetterToVersionHandler MessageType;
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerGetVisitor<MessageType::SourcePersona> get_version_visitor(
      service, Identity(sender.data.string()), message.id);
  boost::apply_visitor(get_version_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service,
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation GetBranchRequestFromDataGetterToVersionHandler";
  typedef nfs::GetBranchRequestFromDataGetterToVersionHandler MessageType;
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerGetBranchVisitor<MessageType::SourcePersona> get_branch_visitor(
      service, message.contents->version_name, Identity(sender.data.string()), message.id);
  boost::apply_visitor(get_branch_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service, const PutVersionRequestFromMaidManagerToVersionHandler& message,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Receiver& /*receiver*/) {
  LOG(kVerbose) << "DoOperation PutVersionRequestFromMaidManagerToVersionHandler";
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerPutVisitor put_version_visitor(service, message.contents->old_version_name,
                                               message.contents->new_version_name,
                                               Identity(sender.group_id.data.string()), message.id);
  boost::apply_visitor(put_version_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service,
    const DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler& message,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Receiver&) {
  LOG(kVerbose) << "DoOperation DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler";
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerDeleteBranchVisitor delete_version_visitor(service, message.contents->version_name,
                                                           Identity(sender.group_id.data.string()));
  boost::apply_visitor(delete_version_visitor, data_name);
}

template <>
void DoOperation(
    VersionHandlerService* service,
    const CreateVersionTreeRequestFromMaidManagerToVersionHandler& message,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename CreateVersionTreeRequestFromMaidManagerToVersionHandler::Receiver&) {
  LOG(kVerbose) << "CreateVersionTreeRequestFromMaidManagerToVersionHandler";
  auto data_name(GetNameVariant(*message.contents));
  VersionHandlerCreateVersionTreeVisitor create_version_tree_visitor(
      service, message.contents->version_name, Identity(sender.group_id.data.string()),
      message.contents->max_versions, message.contents->max_branches, message.id);
  boost::apply_visitor(create_version_tree_visitor, data_name);
}

// ================================================================================================

template <>
template <>
void OperationHandler<typename ValidateSenderType<PutRequestFromDataManagerToPmidManager>::type,
                      Accumulator<PmidManagerServiceMessages>,
                      typename Accumulator<PmidManagerServiceMessages>::AddCheckerFunctor,
                      PmidManagerService>::
operator()(const PutRequestFromDataManagerToPmidManager& message,
           const PutRequestFromDataManagerToPmidManager::Sender& sender,
           const PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  DoOperation(service, message, sender, receiver);
}

template <>
template <>
void OperationHandler<typename ValidateSenderType<GetRequestFromDataManagerToPmidNode>::type,
                      Accumulator<PmidNodeServiceMessages>,
                      typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor,
                      PmidNodeService>::
operator()(const GetRequestFromDataManagerToPmidNode& message,
           const GetRequestFromDataManagerToPmidNode::Sender& sender,
           const GetRequestFromDataManagerToPmidNode::Receiver& receiver) {
  DoOperation(service, message, sender, receiver);
}

template <>
template <>
void OperationHandler<
    typename ValidateSenderType<IntegrityCheckRequestFromDataManagerToPmidNode>::type,
    Accumulator<PmidNodeServiceMessages>,
    typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor, PmidNodeService>::
operator()(const IntegrityCheckRequestFromDataManagerToPmidNode& message,
           const IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
           const IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver) {
  DoOperation(service, message, sender, receiver);
}

//====================================== To MpidManager ===========================================

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::CreateAccountRequestFromMpidNodeToMpidManager& message,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Sender& /*sender*/,
    const typename nfs::CreateAccountRequestFromMpidNodeToMpidManager::Receiver& /*receiver*/) {
  service->HandleCreateAccount(message.contents->public_mpid(), message.contents->public_anmpid(),
                               message.id);
}

template <>
void DoOperation(
    MpidManagerService* service,
    const SendAlertFromMpidManagerToMpidManager& message,
    const typename SendAlertFromMpidManagerToMpidManager::Sender& /*sender*/,
    const typename SendAlertFromMpidManagerToMpidManager::Receiver& receiver) {
  service->HandleMessageAlert(*message.contents, MpidName(Identity(receiver.data.string())));
}

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::GetMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Receiver& /*receiver*/) {
  service->HandleGetMessageRequestFromMpidNode(*message.contents,
                                               MpidName(Identity(sender.data.string())),
                                               message.id);
}

template <>
void DoOperation(
    MpidManagerService* service,
    const GetRequestFromMpidManagerToMpidManager& message,
    const typename GetRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename GetRequestFromMpidManagerToMpidManager::Receiver& /*receiver*/) {
  service->HandleGetMessageRequest(*message.contents,
                                    MpidName(Identity(sender.group_id.data.string())),
                                    message.id);
}

template <>
void DoOperation(
    MpidManagerService* service,
    const GetResponseFromMpidManagerToMpidManager& message,
    const typename GetResponseFromMpidManagerToMpidManager::Sender& /*sender*/,
    const typename GetResponseFromMpidManagerToMpidManager::Receiver& receiver) {
  service->HandleGetMessageResponse(*message.contents, MpidName(Identity(receiver.data.string())),
                                    message.id);
}

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::DeleteRequestFromMpidNodeToMpidManager& message,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Receiver& /*receiver*/) {
  service->HandleDeleteRequest(*message.contents, MpidName(Identity(sender.data.string())));
}

template <>
void DoOperation(
    MpidManagerService* service,
    const DeleteRequestFromMpidManagerToMpidManager& message,
    const typename DeleteRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename DeleteRequestFromMpidManagerToMpidManager::Receiver& receiver) {
  service->HandleDeleteRequest(*message.contents, MpidName(Identity(sender.group_id.data.string())),
                               MpidName(Identity(receiver.data.string())));
}

template <>
void DoOperation(
    MpidManagerService* service,
    const nfs::SendMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Receiver& /*receiver*/) {
  service->HandleSendMessage(*message.contents, MpidName(Identity(sender.data.string())),
                             message.id);
}

template <>
void DoOperation(
    MpidManagerService* service,
    const PutResponseFromDataManagerToMpidManager& message,
    const typename PutResponseFromDataManagerToMpidManager::Sender& /*sender*/,
    const typename PutResponseFromDataManagerToMpidManager::Receiver& receiver) {
  LOG(kVerbose) << "DoOperation PutResponseFromDataManagerToMpidManager";
  auto data_name(GetNameVariant(*message.contents));
  PutResponseVisitor<MpidManagerService, MpidName> put_response_visitor(
      service, Identity(receiver.data.string()), message.contents->cost, message.id);
  boost::apply_visitor(put_response_visitor, data_name);
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
