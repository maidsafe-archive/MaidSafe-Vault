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

#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/maid_manager/dispatcher.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/value.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/account_transfer.pb.h"

namespace maidsafe {

namespace vault {

namespace test {

class MaidManagerServiceTest;

}

class AccountDb;
struct GetPmidTotalsOp;

class MaidManagerService {
 public:
  typedef nfs::MaidManagerServiceMessages PublicMessages;
  typedef MaidManagerServiceMessages VaultMessages;
  typedef void HandleMessageReturnType;
  using Key = MaidManager::Key;
  using Value = MaidManager::Value;
  using AccountType = MaidManager::AccountType;
  using TransferInfo = MaidManager::TransferInfo;

  MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter,
                     const boost::filesystem::path& vault_root_dir);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

  void Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
  }

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

  // =============== Account Creation ==============================================================
  void HandleCreateMaidAccount(const passport::PublicMaid &public_maid,
                               const passport::PublicAnmaid& public_anmaid,
                               nfs::MessageId message_id);
  void HandleSyncedCreateMaidAccount(
      std::unique_ptr<MaidManager::UnresolvedCreateAccount>&& synced_action);

  void HandleSyncedRemoveMaidAccount(
      std::unique_ptr<MaidManager::UnresolvedRemoveAccount>&& synced_action);

  // =============== Put/Delete data ===============================================================
  template <typename Data>
  void HandlePut(const MaidName& account_name, const Data& data, nfs::MessageId message_id);

  template <typename Data>
  void HandlePutResponse(const MaidName& maid_name, const typename Data::Name& data_name,
                         int64_t size, nfs::MessageId message_id);

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

  void TransferAccount(const NodeId& dest, const std::vector<AccountType>& accounts);

  // Only Maid and Anmaid can create account; for all others this is a no-op.
  typedef std::true_type AllowedAccountCreationType;
  typedef std::false_type DisallowedAccountCreationType;
  template <typename Data>
  void CreateAccount(const MaidName& account_name, AllowedAccountCreationType);
  template <typename Data>
  void CreateAccount(const MaidName& /*account_name*/, DisallowedAccountCreationType) {}

  void HandleRemoveAccount(const MaidName& maid_name, nfs::MessageId mesage_id);

  // =========================== Sync / AccountTransfer ============================================
  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);

  void HandleAccountTransfer(const AccountType& account);

  template<typename DataName>
  void HandleAccountRequest(const DataName& name, const NodeId& sender);
  void HandleAccountTransferEntry(const std::string& serialised_account,
                                  const routing::GroupSource& sender);

 private:
  typedef Accumulator<nfs::MaidManagerServiceMessages> NfsAccumulator;
  typedef Accumulator<MaidManagerServiceMessages> VaultAccumulator;

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
  friend class detail::MaidManagerAccountRequestVisitor<MaidManagerService>;
  friend class test::MaidManagerServiceTest;

  routing::Routing& routing_;
  nfs_client::DataGetter& data_getter_;
  std::map<MaidManager::Key, MaidManager::Value> accounts_;
  std::mutex accumulator_mutex_, mutex_;
  bool stopped_;
  NfsAccumulator nfs_accumulator_;
  VaultAccumulator vault_accumulator_;
  MaidManagerDispatcher dispatcher_;
  Sync<MaidManager::UnresolvedCreateAccount> sync_create_accounts_;
  Sync<MaidManager::UnresolvedRemoveAccount> sync_remove_accounts_;
  Sync<MaidManager::UnresolvedPut> sync_puts_;
  Sync<MaidManager::UnresolvedDelete> sync_deletes_;
  AccountTransferHandler<MaidManager> account_transfer_;
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
  const AccountQueryFromMaidManagerToMaidManager& message,
  const typename AccountQueryFromMaidManagerToMaidManager::Sender& sender,
  const typename AccountQueryFromMaidManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
  const AccountQueryResponseFromMaidManagerToMaidManager& message,
  const typename AccountQueryResponseFromMaidManagerToMaidManager::Sender& sender,
  const typename AccountQueryResponseFromMaidManagerToMaidManager::Receiver& receiver);

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
    const typename passport::PublicMaid::Name& data_name, int64_t size,
    nfs::MessageId message_id);

template <>
void MaidManagerService::HandlePutResponse<passport::PublicAnmaid>(const MaidName& maid_name,
    const typename passport::PublicAnmaid::Name& data_name, int64_t size,
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
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerService::HandlePut for account " << HexSubstr(account_name->string())
                << " with data " << HexSubstr(data.name().value)
                << " and message_id " << message_id.data;
  MaidManagerValue value;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it(accounts_.find(account_name));
    if (it == std::end(accounts_))
      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
    value = it->second;
  }
  if (value.AllowPut(data) == MaidManagerValue::Status::kNoSpace) {
    LOG(kWarning) << "MaidManagerService::HandlePut disallowing put";
    dispatcher_.SendPutFailure<Data>(account_name, data.name(),
                                     maidsafe_error(CommonErrors::cannot_exceed_limit),
                                     message_id);
    return;
  }

  dispatcher_.SendPutRequest(account_name, data, message_id);
}

template <typename Data>
void MaidManagerService::HandlePutResponse(const MaidName& maid_name,
    const typename Data::Name& data_name, int64_t size, nfs::MessageId message_id) {
  LOG(kVerbose) << "MaidManagerService::HandlePutResponse to maid "
                << HexSubstr(maid_name->string())
                << " for data name " << HexSubstr(data_name.value)
                << " with size " << size;
  dispatcher_.SendPutResponse(maid_name, maidsafe_error(CommonErrors::success), message_id);
  typename MaidManager::SyncKey group_key(maid_name, data_name, Data::Tag::kValue);
  DoSync(typename MaidManager::UnresolvedPut(group_key, ActionMaidManagerPut(size),
                                             routing_.kNodeId()));
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
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it(accounts_.find(maid_name));
    if (it == std::end(accounts_))
      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  }
  dispatcher_.SendCreateVersionTreeRequest(maid_name, data_name, version, max_versions,
                                           max_branches, message_id);
}

template <typename DataNameType>
void MaidManagerService::HandlePutVersionRequest(
    const MaidName& maid_name, const DataNameType& data_name,
    const StructuredDataVersions::VersionName& old_version,
    const StructuredDataVersions::VersionName& new_version, nfs::MessageId message_id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it(accounts_.find(maid_name));
    if (it == std::end(accounts_))
      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  }
  LOG(kVerbose) << "MaidManagerService::HandlePutVersionRequest put new version "
                << DebugId(new_version.id) << " after old version "
                << DebugId(old_version.id) << " for " << HexSubstr(data_name.value);
  dispatcher_.SendPutVersionRequest(maid_name, data_name, old_version, new_version, message_id);
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
void MaidManagerService::HandleDelete(const MaidName& /*account_name*/,
                                      const typename Data::Name& /*data_name*/,
                                      nfs::MessageId /*message_id*/) {
  //// Only need to ensure that account exist in db. Data name availability in db is not guaranteed
  //// because related put data action may be still syncing
  // LOG(kVerbose) << "MaidManagerService::HandleDelete for account "
  //               << HexSubstr(account_name->string()) << " of chunk "
  //               << HexSubstr(data_name.value);
  // group_db_.GetMetadata(account_name);  // throws
  // typename MaidManager::Key group_key(typename MaidManager::GroupName(account_name.value),
  //                                    data_name, Data::Tag::kValue);
  // DoSync(typename MaidManager::UnresolvedDelete(group_key, ActionMaidManagerDelete(message_id),
  //                                              routing_.kNodeId()));
}

template<typename DataName>
void MaidManagerService::HandleAccountRequest(const DataName& name, const NodeId& sender) {
  protobuf::AccountTransfer account_transfer_proto;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it(accounts_.find(Key(name.value)));
    if (it == std::end(accounts_))
      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));

    protobuf::MaidManagerKeyValuePair kv_pair;
    vault::Key key(it->first.value, MaidManager::Key::data_type::Tag::kValue);
    kv_pair.set_key(key.Serialise());
    kv_pair.set_value(it->second.Serialise());
    account_transfer_proto.add_serialised_accounts(kv_pair.SerializeAsString());
  }
  dispatcher_.SendAccountResponse(account_transfer_proto.SerializeAsString(),
      routing::GroupId(NodeId(name->string())), sender);
}

// ===============================================================================================

template <typename UnresolvedAction>
void MaidManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_create_accounts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_accounts_, unresolved_action);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
