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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_

#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/maid_manager/action_register_pmid.h"
#include "maidsafe/vault/maid_manager/action_unregister_pmid.h"
#include "maidsafe/vault/maid_manager/dispatcher.h"
#include "maidsafe/vault/maid_manager/helpers.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/sync.h"

namespace maidsafe {

class OwnerDirectory;
class GroupDirectory;
class WorldDirectory;

namespace vault {

namespace test {

class MaidManagerServiceTest;

}

class AccountDb;
struct PmidRegistrationOp;
struct GetPmidTotalsOp;

class MaidManagerService {
 public:
  typedef nfs::MaidManagerServiceMessages PublicMessages;
  typedef MaidManagerServiceMessages VaultMessages;
  typedef void HandleMessageReturnType;

  MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

 private:
  static int DefaultPaymentFactor() { return kDefaultPaymentFactor_; }

 private:
  MaidManagerService(const MaidManagerService&);
  MaidManagerService& operator=(const MaidManagerService&);
  MaidManagerService(MaidManagerService&&);
  MaidManagerService& operator=(MaidManagerService&&);

  //  void CheckSenderIsConnectedMaidNode(const nfs::Message& message) const;
  //  void CheckSenderIsConnectedMaidManager(const nfs::Message& message) const;
  //  void ValidateDataSender(const nfs::Message& message) const;
  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const {
  // BEFORE_RELEASE missing function imeplementation
    return true;
  }

  // =============== account creation & pmid registration===========================================
  void HandleCreateMaidAccount(const passport::PublicMaid &public_maid,
                               const passport::PublicAnmaid& public_anmaid,
                               nfs::MessageId message_id);
  void HandleSyncedCreateMaidAccount(
      std::unique_ptr<MaidManager::UnresolvedCreateAccount>&& synced_action);

  void HandleSyncedRemoveMaidAccount(
      std::unique_ptr<MaidManager::UnresolvedRemoveAccount>&& synced_action);

  void HandlePmidRegistration(const nfs_vault::PmidRegistration& pmid_registration);
  void HandlePmidUnregistration(const MaidName& maid_name, const PmidName& pmid_name);

  void HandleSyncedPmidRegistration(
      std::unique_ptr<MaidManager::UnresolvedRegisterPmid>&& synced_action);

  void HandleSyncedPmidUnregistration(
      std::unique_ptr<MaidManager::UnresolvedUnregisterPmid>&& synced_action);

  template<typename PublicFobType>
  void ValidatePmidRegistration(PublicFobType public_fob,
                                std::shared_ptr<PmidRegistrationOp> pmid_registration_op);
  // =============== Put/Delete data ===============================================================
  template <typename Data>
  void HandlePut(const MaidName& account_name, const Data& data, const PmidName& pmid_node_hint,
                 nfs::MessageId message_id);

  template <typename Data>
  void HandlePutResponse(const MaidName& maid_name, const typename Data::Name& data_name,
                         int32_t cost, nfs::MessageId message_id);

  template <typename Data>
  void HandlePutFailure(const MaidName& maid_name, const typename Data::Name& data_name,
                        const maidsafe_error& error, nfs::MessageId message_id);

  void HandleSyncedPutResponse(std::unique_ptr<MaidManager::UnresolvedPut>&& synced_action_put);

  template <typename Data>
  void HandleDelete(const MaidName& account_name, const typename Data::Name& data_name,
                    nfs::MessageId message_id);

  void HandleSyncedDelete(std::unique_ptr<MaidManager::UnresolvedDelete>&& synced_action_delete);

  // ================================== Version Handlers ===========================================

  template <typename DataNameType>
  void HandleCreateVersionTreeRequest(const MaidName& maid_name, const DataNameType& data_name,
                                      const StructuredDataVersions::VersionName& version,
                                      uint32_t max_versions, uint32_t max_branches,
                                      nfs::MessageId message_id);

  void HandleCreateVersionTreeResponse(const MaidName& maid_name, const maidsafe_error& error,
                                       nfs::MessageId message_id);

  template <typename DataNameType>
  void HandlePutVersionRequest(const MaidName& maid_name, const DataNameType& data_name,
                               const StructuredDataVersions::VersionName& old_version,
                               const StructuredDataVersions::VersionName& new_version,
                               nfs::MessageId message_id);

  void HandlePutVersionResponse(const MaidName& maid_name, const maidsafe_error& return_code,
                                std::unique_ptr<StructuredDataVersions::VersionName> tip_of_tree,
                                nfs::MessageId message_id);

  template <typename DataNameType>
  void HandleDeleteBranchUntilFork(const MaidName& maid_name, const DataNameType& data_name,
                                   const StructuredDataVersions::VersionName& version,
                                   nfs::MessageId message_id);

  void HandleIncrementReferenceCounts(const MaidName& maid_name,
                                      const nfs_vault::DataNames& data_names);
  void HandleDecrementReferenceCounts(const MaidName& maid_name,
                                      const nfs_vault::DataNames& data_names);

  void HandleSyncedIncrementReferenceCounts(
      std::unique_ptr<MaidManager::UnresolvedIncrementReferenceCounts>&&
          synced_action_increment_reference_counts);

  void HandleSyncedDecrementReferenceCounts(
      std::unique_ptr<MaidManager::UnresolvedDecrementReferenceCounts>&&
          synced_action_decrement_reference_counts);

  // ===============================================================================================

  void HandleSyncedUpdatePmidHealth(std::unique_ptr<MaidManager::UnresolvedUpdatePmidHealth>&&
                                        synced_action_update_pmid_health);

  void HandleHealthResponse(const MaidName& maid_name, const PmidName& pmid_node,
                            const std::string &serialised_pmid_health,
                            nfs_client::ReturnCode& return_code, nfs::MessageId message_id);

//  MaidManagerMetadata::Status AllowPut(const MaidName& account_name, int32_t cost);

  // Only Maid and Anmaid can create account; for all others this is a no-op.
  typedef std::true_type AllowedAccountCreationType;
  typedef std::false_type DisallowedAccountCreationType;
  template <typename Data>
  void CreateAccount(const MaidName& account_name, AllowedAccountCreationType);
  template <typename Data>
  void CreateAccount(const MaidName& /*account_name*/, DisallowedAccountCreationType) {}
  void FinalisePmidRegistration(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);

  void HandleRemoveAccount(const MaidName& maid_name, nfs::MessageId mesage_id);

  // ===================================== PMID totals ============================================
  void UpdatePmidTotals(const MaidName& account_name);
  void UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                std::shared_ptr<GetPmidTotalsOp> op_data);
  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);

  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   nfs::MaidManagerServiceMessages::types>::type IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   MaidManagerServiceMessages::types>::type FinalType;

 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  typedef Accumulator<nfs::MaidManagerServiceMessages> NfsAccumulator;
  typedef Accumulator<MaidManagerServiceMessages> VaultAccumulator;

  void ObfuscateKey(MaidManager::Key& key) {
    // Hash the data name to obfuscate the list of chunks associated with the client.
    key.name = Identity(crypto::Hash<crypto::SHA512>(key.name));
  }

  bool CheckDataNamesExist(const MaidName& maid_name, const nfs_vault::DataNames& data_names);

  struct MaidAccountCreationStatus {
    MaidAccountCreationStatus(passport::PublicMaid::Name maid_name_in,
                              passport::PublicAnmaid::Name anmaid_name_in)
        : maid_name(std::move(maid_name_in)),
          anmaid_name(std::move(anmaid_name_in)),
          maid_stored(false),
          anmaid_stored(false) {}

    passport::PublicMaid::Name maid_name;
    passport::PublicAnmaid::Name anmaid_name;
    bool maid_stored, anmaid_stored;
  };

  template<typename ServiceHandlerType, typename MessageType>
  friend void detail::DoOperation(
      ServiceHandlerType* service, const MessageType& message,
      const typename MessageType::Sender& sender,
      const typename MessageType::Receiver& receiver);

  friend class detail::MaidManagerPutVisitor<MaidManagerService>;
  friend class detail::MaidManagerPutResponseVisitor<MaidManagerService>;
  friend class detail::MaidManagerPutResponseFailureVisitor<MaidManagerService>;
  friend class detail::MaidManagerDeleteVisitor<MaidManagerService>;
  friend class detail::MaidManagerPutVersionRequestVisitor<MaidManagerService>;
  friend class detail::MaidManagerDeleteBranchUntilForkVisitor<MaidManagerService>;
  friend class detail::MaidManagerCreateVersionTreeRequestVisitor<MaidManagerService>;
  friend class test::MaidManagerServiceTest;

  routing::Routing& routing_;
  nfs_client::DataGetter& data_getter_;
  GroupDb<MaidManager> group_db_;
  std::mutex accumulator_mutex_;
  NfsAccumulator nfs_accumulator_;
  VaultAccumulator vault_accumulator_;
  MaidManagerDispatcher dispatcher_;
  Sync<MaidManager::UnresolvedCreateAccount> sync_create_accounts_;
  Sync<MaidManager::UnresolvedRemoveAccount> sync_remove_accounts_;
  Sync<MaidManager::UnresolvedPut> sync_puts_;
  Sync<MaidManager::UnresolvedDelete> sync_deletes_;
  Sync<MaidManager::UnresolvedRegisterPmid> sync_register_pmids_;
  Sync<MaidManager::UnresolvedUnregisterPmid> sync_unregister_pmids_;
  Sync<MaidManager::UnresolvedUpdatePmidHealth> sync_update_pmid_healths_;
  Sync<MaidManager::UnresolvedIncrementReferenceCounts> sync_increment_reference_counts_;
  Sync<MaidManager::UnresolvedDecrementReferenceCounts> sync_decrement_reference_counts_;
  static const int kDefaultPaymentFactor_;
  std::mutex pending_account_mutex_;
  std::map<nfs::MessageId, MaidAccountCreationStatus> pending_account_map_;
};

template <typename MessageType>
void MaidManagerService::HandleMessage(const MessageType& /*message*/,
                                       const typename MessageType::Sender& /*sender*/,
                                       const typename MessageType::Receiver& /*receiver*/) {
  LOG(kError) << "invalid function call because of un-specialised templated method";
  MessageType::No_generic_handler_is_available__Specialisation_required;
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PutResponseFromDataManagerToMaidManager& message,
    const typename PutResponseFromDataManagerToMaidManager::Sender& sender,
    const typename PutResponseFromDataManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PutFailureFromDataManagerToMaidManager& message,
    const typename PutFailureFromDataManagerToMaidManager::Sender& sender,
    const typename PutFailureFromDataManagerToMaidManager::Receiver& receiver);


template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::UnregisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PmidHealthResponseFromPmidManagerToMaidManager& message,
    const typename PmidHealthResponseFromPmidManagerToMaidManager::Sender& sender,
    const typename PmidHealthResponseFromPmidManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const SynchroniseFromMaidManagerToMaidManager& message,
    const typename SynchroniseFromMaidManagerToMaidManager::Sender& sender,
    const typename SynchroniseFromMaidManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const AccountTransferFromMaidManagerToMaidManager& message,
    const typename AccountTransferFromMaidManagerToMaidManager::Sender& sender,
    const typename AccountTransferFromMaidManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::IncrementReferenceCountsFromMaidNodeToMaidManager& message,
    const typename nfs::IncrementReferenceCountsFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::IncrementReferenceCountsFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::DecrementReferenceCountsFromMaidNodeToMaidManager& message,
    const typename nfs::DecrementReferenceCountsFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DecrementReferenceCountsFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PutVersionResponseFromVersionHandlerToMaidManager& message,
    const typename PutVersionResponseFromVersionHandlerToMaidManager::Sender& sender,
    const typename PutVersionResponseFromVersionHandlerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateVersionTreeRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const CreateVersionTreeResponseFromVersionHandlerToMaidManager& message,
    const typename CreateVersionTreeResponseFromVersionHandlerToMaidManager::Sender& sender,
    const typename CreateVersionTreeResponseFromVersionHandlerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandlePutResponse<passport::PublicMaid>(const MaidName& maid_name,
    const typename passport::PublicMaid::Name& data_name, int32_t,
    nfs::MessageId message_id);

template <>
void MaidManagerService::HandlePutResponse<passport::PublicAnmaid>(const MaidName& maid_name,
    const typename passport::PublicAnmaid::Name& data_name, int32_t,
    nfs::MessageId message_id);

// ==================== Implementation =============================================================
namespace detail {

template <typename T>
struct can_create_account : public std::false_type {};

template <>
struct can_create_account<passport::PublicAnmaid> : public std::true_type {};

template <>
struct can_create_account<passport::PublicMaid> : public std::true_type {};

}  // namespace detail

// ================================== Put Implementation ========================================

template <typename Data>
void MaidManagerService::HandlePut(const MaidName& account_name, const Data& data,
                                   const PmidName& pmid_node_hint,
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerService::HandlePut for account " << HexSubstr(account_name->string())
                << " with data " << HexSubstr(data.name().value)
                << " and pmid_node_hint " << HexSubstr(pmid_node_hint->string())
                << " message_id " << message_id.data;
  try {
    auto metadata(group_db_.GetMetadata(account_name));
    if (metadata.AllowPut(data) != MaidManagerMetadata::Status::kNoSpace) {
      LOG(kInfo) << "MaidManagerService::HandlePut allowing put";
      typename MaidManager::Key group_key(typename MaidManager::GroupName(account_name.value),
                                          data.name(), Data::Tag::kValue);
      auto obfuscated_key(group_key);
      ObfuscateKey(obfuscated_key);
      try {
        group_db_.GetValue(obfuscated_key);
        // BEFORE_RELEASE putting a duplicated chunk, cost set to the size of the data
        LOG(kInfo) << "MaidManagerService::HandlePut duplicated PutRequest";
        DoSync(typename MaidManager::UnresolvedPut(group_key,
            ActionMaidManagerPut(static_cast<int32_t>(data.Serialise().data.string().size())),
            routing_.kNodeId()));
        return;
      } catch(const maidsafe_error& error) {
        LOG(kInfo) << "MaidManagerService::HandlePut first PutRequest, passing to DataManager: "
                   << error.what();
      }
      dispatcher_.SendPutRequest(account_name, data, pmid_node_hint, message_id);
    } else {
      LOG(kWarning) << "MaidManagerService::HandlePut disallowing put";
      dispatcher_.SendPutFailure<Data>(account_name, data.name(),
                                      maidsafe_error(CommonErrors::cannot_exceed_limit),
                                      message_id);
    }
  } catch(const maidsafe_error& error) {
    LOG(kError) << "MaidManagerService::HandlePut getting metadata has error : " << error.what();
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

template <typename Data>
void MaidManagerService::HandlePutResponse(const MaidName& maid_name,
                                           const typename Data::Name& data_name,
                                           int32_t cost, nfs::MessageId /*message_id*/) {
  LOG(kVerbose) << "MaidManagerService::HandlePutResponse to maid "
                << HexSubstr(maid_name->string())
                << " for data name " << HexSubstr(data_name.value)
                << " taking cost of " << cost;
  typename MaidManager::Key group_key(typename MaidManager::GroupName(maid_name.value),
                                      data_name, Data::Tag::kValue);
  DoSync(typename MaidManager::UnresolvedPut(group_key,
                                             ActionMaidManagerPut(cost), routing_.kNodeId()));
}

template <typename Data>
void MaidManagerService::HandlePutFailure(
    const MaidName& maid_name, const typename Data::Name& data_name,
    const maidsafe_error& error, nfs::MessageId message_id) {
  dispatcher_.SendPutFailure<Data>(maid_name, data_name, error, message_id);
}

template <typename DataNameType>
void MaidManagerService::HandleCreateVersionTreeRequest(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& version, uint32_t max_versions,
    uint32_t max_branches, nfs::MessageId message_id) {
  try {
    group_db_.GetMetadata(maid_name);
    dispatcher_.SendCreateVersionTreeRequest(maid_name, data_name, version, max_versions,
                                             max_branches, message_id);
  }
  catch (const maidsafe_error& error) {
    LOG(kError) << "MaidManagerService::HandleCreateVersionTreeRequest faied: " << error.what();
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

template <typename DataNameType>
void MaidManagerService::HandlePutVersionRequest(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& old_version,
    const StructuredDataVersions::VersionName& new_version, nfs::MessageId message_id) {
  try {
    group_db_.GetMetadata(maid_name);
    dispatcher_.SendPutVersionRequest(maid_name, data_name, old_version, new_version, message_id);
  }
  catch (const maidsafe_error& error) {
    LOG(kError) << "MaidManagerService::HandlePutVersion faied to get metadata" << error.what();
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      throw;
  }
}

template <typename DataNameType>
void MaidManagerService::HandleDeleteBranchUntilFork(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& version, nfs::MessageId message_id) {
  dispatcher_.SendDeleteBranchUntilFork(maid_name, data_name, version, message_id);
}

template <>
void MaidManagerService::HandlePutFailure<passport::PublicMaid>(
    const MaidName& maid_name, const passport::PublicMaid::Name& data_name,
    const maidsafe_error& error, nfs::MessageId message_id);

template <>
void MaidManagerService::HandlePutFailure<passport::PublicAnmaid>(
    const MaidName& maid_name, const passport::PublicAnmaid::Name& data_name,
    const maidsafe_error& error, nfs::MessageId message_id);

// ================================== Delete Implementation =======================================

template <typename Data>
void MaidManagerService::HandleDelete(const MaidName& account_name,
                                      const typename Data::Name& data_name,
                                      nfs::MessageId message_id) {
  // Only need to ensure that account exist in db. Data name availability in db is not guaranteed
  // because related put data action may be still syncing
  LOG(kVerbose) << "MaidManagerService::HandleDelete for account "
                << HexSubstr(account_name->string()) << " of chunk " << HexSubstr(data_name.value);
  group_db_.GetMetadata(account_name);  // throws
  typename MaidManager::Key group_key(typename MaidManager::GroupName(account_name.value),
                                      data_name, Data::Tag::kValue);
  DoSync(typename MaidManager::UnresolvedDelete(group_key, ActionMaidManagerDelete(message_id),
                                                routing_.kNodeId()));
}

// ===============================================================================================

template<typename PublicFobType>
void MaidManagerService::ValidatePmidRegistration(
    PublicFobType public_fob,
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
  LOG(kVerbose) << "MaidManagerService::ValidatePmidRegistration";
  bool finalise(false);
  {
    std::lock_guard<std::mutex> lock(pmid_registration_op->mutex);
    pmid_registration_op->SetPublicFob(std::move(public_fob));
    finalise = (++pmid_registration_op->count == 2);
    LOG(kVerbose) << "MaidManagerService::ValidatePmidRegistration "
                  << " pmid_registration_op->count " << pmid_registration_op->count
                  << " finalised " << finalise;
  }
  if (finalise)
    FinalisePmidRegistration(pmid_registration_op);
}

template <typename UnresolvedAction>
void MaidManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_accounts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_accounts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_register_pmids_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_unregister_pmids_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_update_pmid_healths_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_increment_reference_counts_,
                                       unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_decrement_reference_counts_,
                                       unresolved_action);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
