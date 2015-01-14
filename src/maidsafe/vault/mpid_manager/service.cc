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

#include <string>

#include "maidsafe/vault/mpid_manager/service.h"
#include "maidsafe/vault/operation_handlers.h"

namespace maidsafe {

namespace vault {

MpidManagerService::MpidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                                       nfs_client::DataGetter& data_getter,
                                       const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      asio_service_(2),
      data_getter_(data_getter),
      accumulator_mutex_(),
      dispatcher_(routing),
      db_(vault_root_dir),
      account_transfer_(),
      sync_alerts_(NodeId(pmid.name()->string())) {}

template <>
void MpidManagerService::HandleMessage(const MessageAlertFromMpidManagerToMpidManager &message,
    const typename MessageAlertFromMpidManagerToMpidManager::Sender& sender,
    const typename MessageAlertFromMpidManagerToMpidManager::Receiver& receiver) {
  using MessageType = MessageAlertFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const nfs::GetMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetMessageRequestFromMpidNodeToMpidManager::Receiver& receiver) {
  using  MessageType = nfs::GetMessageRequestFromMpidNodeToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

template <>
void MpidManagerService::HandleMessage(
    const GetMessageRequestFromMpidManagerToMpidManager& message,
    const typename GetMessageRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename GetMessageRequestFromMpidManagerToMpidManager::Receiver& receiver) {
  using  MessageType = GetMessageRequestFromMpidManagerToMpidManager;
  OperationHandlerWrapper<MpidManagerService, MessageType>(
      accumulator_, [this](const MessageType& message, const MessageType::Sender& sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)),
      this, accumulator_mutex_)(message, sender, receiver);
}

void MpidManagerService::HandleMessageAlert(const nfs_vault::MpidMessageAlert& alert,
                                            const MpidName& receiver) {
  if (!AccountExists(receiver))
    return;

  DoSync(MpidManager::UnresolvedMessageAlert(
      MpidManager::SyncGroupKey(receiver),
      ActionMpidManagerMessageAlert(alert), routing_.kNodeId()));
  if (IsOnline(receiver))
    dispatcher_.SendMessageAlert(alert, receiver);
}

void MpidManagerService::HandleGetMessageRequestFromMpidNode(
    const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver) {
  if (!AlertExists(alert, receiver))
    return;

  dispatcher_.SendGetMessageRequest(alert, receiver);
}

void MpidManagerService::HandleGetMessageRequest(const nfs_vault::MpidMessageAlert& /*alert*/,
                                                 const MpidName& /*receiver*/) {
}

bool MpidManagerService::IsOnline(const MpidName& /*mpid_name*/) {
  return true;
}

bool MpidManagerService::AccountExists(const MpidName& /*mpid_name*/) {
  return true;
}

bool MpidManagerService::AlertExists(const nfs_vault::MpidMessageAlert& /*alert*/,
                                     const MpidName& /*receiver*/) {
  return true;
}

}  // namespace vault

}  // namespace maidsafe
