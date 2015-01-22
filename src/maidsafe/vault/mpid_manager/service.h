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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_SERVICE_H_

#include <vector>

#include "boost/expected/expected.hpp"
#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/rsa.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/mpid_manager/mpid_manager.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/mpid_manager/database.h"
#include "maidsafe/vault/mpid_manager/dispatcher.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class MpidManagerService {
 public:
  using PublicMessages = nfs::MpidManagerServiceMessages;
  using VaultMessages = MpidManagerServiceMessages;
  using HandleMessageReturnType = void;

  MpidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter,
                     const boost::filesystem::path& vault_root_dir);
  ~MpidManagerService();

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);
  void HandleChurnEvent(std::shared_ptr<routing::ClientNodesChange> client_nodes_change);

  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);

 private:
  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   PublicMessages::types>::type IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   VaultMessages::types>::type FinalType;

 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  template<typename ServiceHandlerType, typename MessageType>
  friend void detail::DoOperation(
      ServiceHandlerType* service, const MessageType& message,
      const typename MessageType::Sender& sender,
      const typename MessageType::Receiver& receiver);

 private:
  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const;

  bool IsOnline(const MpidName& mpid_name);  // TO BE IMPLEMENTED

  void HandleSendMessage(const nfs_vault::MpidMessage& message, const MpidName& sender);
  void HandleMessageAlert(const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver);
  void HandleGetMessageRequestFromMpidNode(const nfs_vault::MpidMessageAlert& alert,
                                           const MpidName& receiver);
  void HandleGetMessageRequest(const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver);
  void HandleGetMessageResponse(const nfs_client::MpidMessageOrReturnCode& message,
                                const MpidName& receiver);

  void HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver);

  void HandleDeleteRequest(const nfs_vault::MpidMessageAlert& alert, const MpidName& receiver,
                           const MpidName& sender);

  routing::Routing& routing_;
  nfs_client::DataGetter& data_getter_;
  mutable std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  routing::CloseNodesChange close_nodes_change_;
  MpidManagerDispatcher dispatcher_;
  MpidManagerDataBase db_;
  AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kMpidManager>> account_transfer_;
  Sync<MpidManager::UnresolvedPutAlert> sync_put_alerts_;
  Sync<MpidManager::UnresolvedDeleteAlert> sync_delete_alerts_;
  Sync<MpidManager::UnresolvedPutMessage> sync_put_messages_;
  Sync<MpidManager::UnresolvedDeleteMessage> sync_delete_messages_;
};

template <typename MessageType>
bool MpidManagerService::ValidateSender(const MessageType& /*message*/,
                                        const typename MessageType::Sender& /*sender*/) const {
  return true;
}

template <typename UnresolvedAction>
void MpidManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_put_alerts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_delete_alerts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_put_messages_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_delete_messages_, unresolved_action);
}

template <typename MessageType>
void HandleMessage(const MessageType&, const typename MessageType::Sender&,
                   const typename MessageType::Receiver&) {
  MessageType::No_generic_handler_is_available__Specialisation_required;
}


template <>
void MpidManagerService::HandleMessage(
    const SendAlertFromMpidManagerToMpidManager& message,
    const typename SendAlertFromMpidManagerToMpidManager::Sender& sender,
    const typename SendAlertFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const nfs::GetRequestFromMpidNodeToMpidManager& message,
    const typename nfs::GetRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::GetRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const GetRequestFromMpidManagerToMpidManager& message,
    const typename GetRequestFromMpidManagerToMpidManager::Sender& sender,
    const typename GetRequestFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const GetResponseFromMpidManagerToMpidManager& message,
    const typename GetResponseFromMpidManagerToMpidManager::Sender& sender,
    const typename GetResponseFromMpidManagerToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMpidNodeToMpidManager& message,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const nfs::SendMessageRequestFromMpidNodeToMpidManager& message,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Sender& sender,
    const typename nfs::SendMessageRequestFromMpidNodeToMpidManager::Receiver& receiver);

template <>
void MpidManagerService::HandleMessage(
    const SynchroniseFromMpidManagerToMpidManager& message,
    const typename SynchroniseFromMpidManagerToMpidManager::Sender& sender,
    const typename SynchroniseFromMpidManagerToMpidManager::Receiver& receiver);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_SERVICE_H_
