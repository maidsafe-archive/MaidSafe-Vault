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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_

#include <mutex>
#include <set>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/pmid_manager/handler.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"


namespace maidsafe {
namespace vault {

class PmidManagerService {
 public:
  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing, Db& db);

  template<typename T>
  void HandleMessage(const T& message,
                     const typename T::Sender& sender,
                     const typename T::Receiver& receiver);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

  template<typename T>
  bool ValidateSender(const T& message, const typename T::Sender& sender) const;

 private:
  PmidManagerService(const PmidManagerService&);
  PmidManagerService& operator=(const PmidManagerService&);
  PmidManagerService(PmidManagerService&&);
  PmidManagerService& operator=(PmidManagerService&&);

  void CreatePmidAccount(
      const nfs::CreateAccountRequestFromMaidManagerToPmidManager& message,
      const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Sender& sender,
      const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);
  void GetPmidTotals(const nfs::Message& message);
  void GetPmidAccount(const nfs::Message& message);

// =============== Put/Delete data ================================================================
  template<typename Data>
  void HandlePut(const Data& data, const PmidName& pmid_node, const nfs::MessageId& message_id);

  template<typename T>
  void HandleDelete(const T& message,
                    const typename T::Sender& sender,
                    const typename T::Receiver& receiver);

  void DoSync();

//  template<typename Data>
//  void HandlePutCallback(const std::string& reply, const nfs::Message& message);
//  template<typename Data>
//  void SendPutResult(const nfs::Message& message, bool result);

  // =============== Account transfer ==============================================================
  void TransferAccount(const PmidName& account_name, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::Message& message);

  void ValidateMessage(const nfs::Message& message) const;

  template<typename Data, nfs::MessageAction action>
  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  Accumulator<nfs::PmidManagerServiceMessages> accumulator_;
  PmidAccountHandler pmid_account_handler_;
  PmidManagerDispatcher dispatcher_;
  Sync<PmidManager::UnresolvedPut> sync_puts_;
};

// ============================= Handle Message Specialisations ===================================

template<typename T>
void PmidManagerService::HandleMessage(const T& /*message*/,
                                       const typename T::Sender& /*sender*/,
                                       const typename T::Receiver& /*receiver*/) {
  T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
}

template<>
void PmidManagerService::HandleMessage(
    const nfs::PutRequestFromDataManagerToPmidManager& message,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::PutResponseFromPmidNodeToPmidManager& message,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Sender& sender,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::DeleteRequestFromDataManagerToPmidManager& message,
    const typename nfs::DeleteRequestFromDataManagerToPmidManager::Sender& sender,
    const typename nfs::DeleteRequestFromDataManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::SynchroniseFromPmidManagerToPmidManager& message,
    const typename nfs::SynchroniseFromPmidManagerToPmidManager::Sender& sender,
    const typename nfs::SynchroniseFromPmidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::AccountTransferFromPmidManagerToPmidManager& message,
    const typename nfs::AccountTransferFromPmidManagerToPmidManager::Sender& sender,
    const typename nfs::AccountTransferFromPmidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidManagerToPmidManager& message,
    const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);


// ================================= Put Implementation ===========================================

template<>
void PmidManagerService::HandleMessage(
    const nfs::PutRequestFromDataManagerToPmidManager& message,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename nfs::PutRequestFromDataManagerToPmidManager::Receiver& receiver) {
  typedef nfs::PutRequestFromDataManagerToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType, nfs::PmidManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<typename Data>
void PmidManagerService::HandlePut(const Data& data,
                                   const PmidName& pmid_node,
                                   const nfs::MessageId& message_id) {
  dispatcher_.SendPutRequest(data, pmid_node, message_id);
  typename PmidManager::Key group_key(PmidManager::GroupName(pmid_node),
                                      data.name(),
                                      Data::Name::data_type);
  sync_puts_.AddLocalAction(
      typename PmidManager::UnresolvedPut(group_key,
                                          ActionPmidManagerPut(data.data().string().size())));
  DoSync();
}

template<>
void PmidManagerService::HandleMessage(
    const nfs::PutResponseFromPmidNodeToPmidManager& message,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Sender& sender,
    const typename nfs::PutResponseFromPmidNodeToPmidManager::Receiver& receiver) {
  typedef nfs::PutResponseFromPmidNodeToPmidManager MessageType;
  OperationHandlerWrapper<PmidManagerService, MessageType, nfs::PmidManagerServiceMessages>(-+
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidManagerServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<typename Data>
void PmidManagerService::HandlePutResponse(const Data& data,
                                           const nfs::MessageId& message_id,
                                           const PmidName& pmid_node,
                                           const maidsafe_error& error) {
  // DIFFERENT ERRORS MUST BE HANDLED DIFFERENTLY
  dispatcher_.SendPutResponse(data, pmid_node, message_id, error);
}

// ===============================================================================================

}  // namespace vault
}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
