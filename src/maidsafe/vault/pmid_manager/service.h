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
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/action_delete.h"
#include "maidsafe/vault/pmid_manager/dispatcher.h"
#include "maidsafe/vault/pmid_manager/handler.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {
namespace vault {

class PmidManagerService {
 public:
  typedef void PublicMessages;
  typedef PmidManagerServiceMessages VaultMessages;
  typedef PmidManagerServiceMessages Messages;

  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing);

  template <typename T>
  void HandleMessage(const T& message, const typename T::Sender& sender,
                     const typename T::Receiver& receiver);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}

  template <typename T>
  bool ValidateSender(const T& /*message*/, const typename T::Sender& /*sender*/) const {
    return false;
  }

 private:
  PmidManagerService(const PmidManagerService&);
  PmidManagerService& operator=(const PmidManagerService&);
  PmidManagerService(PmidManagerService&&);
  PmidManagerService& operator=(PmidManagerService&&);

  void CreatePmidAccount(
      const CreateAccountRequestFromMaidManagerToPmidManager& message,
      const CreateAccountRequestFromMaidManagerToPmidManager::Sender& sender,
      const CreateAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);
  //  void GetPmidTotals(const nfs::Message& message);
  //  void GetPmidAccount(const nfs::Message& message);

  // =============== Put/Delete data =============================================================
  template <typename Data>
  void HandlePut(const Data& data, const PmidName& pmid_node, const nfs::MessageId& message_id);

  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data,
                        const PmidName& pmid_node,
                        const int64_t& available_space,
                        const maidsafe_error& error_code,
                        const nfs::MessageId& message_id);
  template <typename Data>
  void HandleDelete(const PmidName& pmid_node, const typename Data::Name& data_name,
                    const nfs::MessageId& message_id);
  void DoSync();

  //  template<typename Data>
  //  void HandlePutCallback(const std::string& reply, const nfs::Message& message);
  //  template<typename Data>
  //  void SendPutResult(const nfs::Message& message, bool result);

  // =============== Account transfer ==============================================================
  //  void TransferAccount(const PmidName& account_name, const NodeId& new_node);
  //  void HandleAccountTransfer(const nfs::Message& message);

  //  void ValidateMessage(const nfs::Message& message) const;

  //  template<typename Data, nfs::MessageAction action>
  //  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message);

  routing::Routing& routing_;
  GroupDb<PmidManager> group_db_;
  std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  PmidManagerDispatcher dispatcher_;
  std::map<PmidName, PmidManagerMetadata> pmid_metadata_;
  Sync<PmidManager::UnresolvedPut> sync_puts_;
  Sync<PmidManager::UnresolvedDelete> sync_deletes_;
};

// ============================= Handle Message Specialisations ===================================

template <typename T>
void PmidManagerService::HandleMessage(const T& /*message*/, const typename T::Sender& /*sender*/,
                                       const typename T::Receiver& /*receiver*/) {
  // T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
}

template <>
void PmidManagerService::HandleMessage(
    const PutRequestFromDataManagerToPmidManager& message,
    const typename PutRequestFromDataManagerToPmidManager::Sender& sender,
    const typename PutRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const PutFailureFromPmidNodeToPmidManager& message,
    const typename PutFailureFromPmidNodeToPmidManager::Sender& sender,
    const typename PutFailureFromPmidNodeToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const DeleteRequestFromDataManagerToPmidManager& message,
    const typename DeleteRequestFromDataManagerToPmidManager::Sender& sender,
    const typename DeleteRequestFromDataManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

// template<>
// void PmidManagerService::HandleMessage(
//    const nfs::AccountTransferFromPmidManagerToPmidManager& message,
//    const typename nfs::AccountTransferFromPmidManagerToPmidManager::Sender& sender,
//    const typename nfs::AccountTransferFromPmidManagerToPmidManager::Receiver& receiver);

template <>
void PmidManagerService::HandleMessage(
    const CreateAccountRequestFromMaidManagerToPmidManager& message,
    const typename CreateAccountRequestFromMaidManagerToPmidManager::Sender& sender,
    const typename CreateAccountRequestFromMaidManagerToPmidManager::Receiver& receiver);

template<>
void PmidManagerService::HandleMessage(
    const SynchroniseFromPmidManagerToPmidManager& message,
    const typename SynchroniseFromPmidManagerToPmidManager::Sender& sender,
    const typename SynchroniseFromPmidManagerToPmidManager::Receiver& receiver);


// ================================= Put Implementation ===========================================

template <typename Data>
void PmidManagerService::HandlePut(const Data& data, const PmidName& pmid_node,
                                   const nfs::MessageId& message_id) {
  dispatcher_.SendPutRequest(data, pmid_node, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), data.name().raw_name,
                             data.name().type);
  sync_puts_.AddLocalAction(
      PmidManager::UnresolvedPut(group_key, ActionPmidManagerPut(data.data().string().size()),
                                 routing_.kNodeId(), message_id));
  DoSync();
}

template <typename Data>
void PmidManagerService::HandlePutFailure(
    const typename Data::Name& name, const PmidName& pmid_node, const int64_t& available_space,
    const maidsafe_error& error_code, const nfs::MessageId& message_id) {
  pmid_metadata_.at(pmid_node).claimed_available_size = available_space;
  dispatcher_.SendPutFailure<Data>(name, pmid_node, error_code, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), name.raw_name, name.type);
  sync_deletes_.AddLocalAction(PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(),
                                                             routing_.kNodeId(), message_id));
  DoSync();
}

template <typename Data>
void PmidManagerService::HandleDelete(
    const PmidName& pmid_node, const typename Data::Name& data_name,
    const nfs::MessageId& message_id) {
  dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
  PmidManager::Key group_key(PmidManager::GroupName(pmid_node), data_name.name().raw_name,
                             data_name.name().type);
  sync_deletes_.AddLocalAction(
      PmidManager::UnresolvedDelete(group_key, ActionPmidManagerDelete(), routing_.kNodeId(),
                                    message_id));
  DoSync();
}

// ===============================================================================================

}  // namespace vault
}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_SERVICE_H_
