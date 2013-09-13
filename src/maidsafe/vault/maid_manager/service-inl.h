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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_


#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"


namespace maidsafe {
namespace vault {

template<>
void MaidManagerService::HandleMessage(
    const nfs::PutRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::PutRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void HandleMessage(
    const nfs::PutResponseFromDataManagerToMaidManager& message,
    const typename nfs::PutResponseFromDataManagerToMaidManager::Sender& sender,
    const typename nfs::PutResponseFromDataManagerToMaidManager::Receiver& receiver) {
  typedef nfs::PutResponseFromDataManagerToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}


template <typename Data>
void MaidManagerService::HandlePutResponse(const MaidName& maid_name,
                                           const Data::Name& data_name,
                                           const int32_t& cost) {
  nfs::PersonaTypes<Persona::kMaidManager>::Key group_key(
      nfs::PersonaTypes<Persona::kMaidManager>::GroupName(maid_name.data),
      data_name(),
      Data::Name::data_type);
  sync_puts_.AddLocalAction(nfs::UnresolvedPut(group_key, ActionMaidManagerPut(cost)));
  DoSync();
}


template <typename Data>
void MaidManagerService::HandlePut(const MaidName& account_name,
                                   const Data& data,
                                   const PmidName& pmid_node_hint,
                                   const nfs::MessageId& message_id) {
  dispatcher_.SendPutRequest(account_name, data, pmid_node_hint, message_id);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::DeleteRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>::value),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template <typename Data>
void MaidManagerService::HandleDelete(const NodeId& account_name,
                                      const typename Data::Name& data_name,
                                      const nfs::MessageId& message_id) {
  dispatcher_.SendDeleteRequest(MaidName(account_name), data_name, message_id);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::PutVersionRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::CreateAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::RemoveAccountRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>::value),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::RegisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::UnregisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::UnregisterPmidRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void MaidManagerService::HandleMessage(
    const nfs::GetPmidHealthRequestFromMaidNodeToMaidManager& message,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Receiver& receiver) {
  typedef nfs::GetPmidHealthRequestFromMaidNodeToMaidManager MessageType;
  OperationHandlerWrapper<MaidManagerService,
                          MessageType,
                          nfs::MaidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::MaidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}


//template <typename Data>
//void MaidManagerService::Sync(const Identity& data_name,
//                              const NodeId& sender,
//                              const nfs::MessageAction& action) {
//  nfs::PersonaTypes<Persona::kMaidManager>::Key group_key(
//      nfs::PersonaTypes<Persona::kMaidManager>::GroupName(sender),
//      data_name,
//      Data::Name::data_type);
//  nfs::UnresolvedPut unresolved_action(group_key, action);
//  sync_puts_.AddLocalAction(unresolved_action);
//  DoSync();
//}




}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_INL_H_
