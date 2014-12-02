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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/message_types_partial.h"

#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/memory_fifo.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/data_manager/dispatcher.h"
#include "maidsafe/vault/data_manager/helpers.h"
#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/database.h"
#include "maidsafe/vault/account_transfer.pb.h"

namespace maidsafe {

namespace vault {

namespace test {

  class DataManagerServiceTest;

}

class DataManagerService {
 public:
  typedef nfs::DataManagerServiceMessages PublicMessages;
  typedef DataManagerServiceMessages VaultMessages;
  typedef void HandleMessageReturnType;
  using AccountType = std::pair<Key, DataManagerValue>;

  DataManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter,
                     const boost::filesystem::path& vault_root_dir);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

  void Stop() {
    std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
    stopped_ = true;
  }

 private:
  DataManagerService(const DataManagerService&);
  DataManagerService& operator=(const DataManagerService&);
  DataManagerService(DataManagerService&&);
  DataManagerService& operator=(DataManagerService&&);

  // =========================== Put section =======================================================
  template <typename Data>
  void HandlePut(const Data& data, const MaidName& maid_name, nfs::MessageId message_id);

  template <typename Data>
  bool EntryExist(const typename Data::Name& name);

  typedef std::true_type EntryMustBeUnique;
  typedef std::false_type EntryNeedNotBeUnique;
  template <typename Data>
  void HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                 nfs::MessageId message_id, uint64_t cost, EntryMustBeUnique);
  template <typename Data>
  void HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                 nfs::MessageId message_id, uint64_t cost, EntryNeedNotBeUnique);

  template <typename Data>
  void HandlePutResponse(const typename Data::Name& data_name, const PmidName& pmid_node,
                         nfs::MessageId message_id);

  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data_name, uint64_t size,
                        const PmidName& attempted_pmid_node, nfs::MessageId message_id,
                        const maidsafe_error& error);

  template <typename DataName>
  bool SendPutRetryRequired(const DataName& data_name);

  template <typename Data>
  void SendPmidUpdateAccount(const typename Data::Name& data_name, const PmidName& pmid_node,
                             uint64_t chunk_size, uint64_t given_size);

  // =========================== Get section (includes integrity checks) ===========================
  typedef GetResponseFromPmidNodeToDataManager::Contents GetResponseContents;

  template <typename Data, typename RequestorIdType>
  void HandleGet(const typename Data::Name& data_name, const RequestorIdType& requestor,
                 nfs::MessageId message_id);

  template <typename Data>
  void GetForReplication(const PmidName& pmid_name, const typename Data::Name& data_name);
  template <typename Data>
  void DoGetForReplication(const typename Data::Name& data_name,
                           const std::set<PmidName>& online_pmids);

  void HandleGetResponse(const PmidName& pmid_name, nfs::MessageId message_id,
                         const GetResponseContents& contents);

  // Removes a pmid_name from the set and returns it.
  template <typename DataName>
  PmidName ChoosePmidNodeToGetFrom(std::set<PmidName>& online_pmids,
                                   const DataName& data_name) const;
  template <typename Data>
  std::set<PmidName> GetOnlinePmids(const typename Data::Name& data_name);

  template <typename Data, typename RequestorIdType>
  void DoHandleGetResponse(
      const PmidName& pmid_node, const GetResponseContents& contents,
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data>
  void DoGetResponseForReplication(const PmidName& pmid_node, const typename Data::Name& data_name,
                                   const GetResponseContents& contents);

  template <typename Data, typename RequestorIdType>
  bool SendGetResponse(
      const Data& data,
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data, typename RequestorIdType>
  void AssessIntegrityCheckResults(
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data, typename RequestorIdType>
  void AssessGetContentRequestedPmidNode(
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  void DerankPmidNode(const PmidName& pmid_node);

  template <typename Data>
  void DeletePmidNodeAsHolder(const PmidName pmid_node, const typename Data::Name& name,
                              nfs::MessageId message_id);

  // =========================== Delete section ====================================================
  template <typename Data>
  void HandleDelete(const typename Data::Name& data_name, nfs::MessageId message_id);

  void SendDeleteRequests(const DataManager::Key& key, const std::set<PmidName>& pmids,
                          nfs::MessageId message_id);

  template <typename Data>
  void SendDeleteRequest(const PmidName pmid_node, const typename Data::Name& name,
                         const uint64_t size, nfs::MessageId message_id);

  uint64_t Replicate(const DataManager::Key& key, nfs::MessageId message_id,
                     const PmidName& tried_pmid_name = PmidName());

  template <typename Data>
  void HandleSendPutRequest(const PmidName& pmid_name, const Data& data, nfs::MessageId);

  template <typename Data>
  void SendFalseDataNotification(const PmidName pmid_node, const typename Data::Name& name,
                                 uint64_t size, nfs::MessageId message_id);

  // =========================== Node up / Node down section =======================================
  template <typename DataName>
  void MarkNodeDown(const PmidName& pmid_node, const DataName& data_name);

  template <typename DataName>
  void MarkNodeUp(const PmidName& pmid_node, const DataName& data_name);

  // =========================== Sync / AccountTransfer section ====================================
  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);

  void TransferAccount(const NodeId& dest,
                       const std::vector<Db<DataManager::Key,
                                         DataManager::Value>::KvPair>& accounts);

  void HandleAccountTransfer(const AccountType& account);

  template<typename DataName>
  void HandleAccountQuery(const DataName& name, const NodeId& sender);
  void HandleAccountTransferEntry(const std::string& serialised_account,
                                  const routing::SingleSource& sender);
  // =========================== General functions =================================================
  void HandleDataIntegrityResponse(const GetResponseContents& response, nfs::MessageId message_id);

  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const {
    // Don't need to check sender or receiver type - only need to check for group sources that
    // sender ID is appropriate (i.e. == MaidName, or == DataName).
    // BEFORE_RELEASE implementation missing
    return true;
  }

  typedef boost::mpl::vector<> InitialType;
  typedef boost::variant<
      nfs::GetRequestFromDataGetterPartialToDataManager> DataManagerServicePartialMessages;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   DataManagerServicePartialMessages::types>::type PartialType;
  typedef boost::mpl::insert_range<PartialType,
                                   boost::mpl::end<PartialType>::type,
                                   nfs::DataManagerServiceMessages::types>::type IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   DataManagerServiceMessages::types>::type FinalType;

 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  template<typename ServiceHandlerType, typename MessageType>
  friend void detail::DoOperation(
      ServiceHandlerType* service, const MessageType& message,
      const typename MessageType::Sender& sender,
      const typename MessageType::Receiver& receiver);

  template <typename ServiceHandlerType, typename RequestorIdType>
  friend class detail::GetRequestVisitor;

  friend class detail::DataManagerPutVisitor<DataManagerService>;
  friend class detail::DataManagerPutResponseVisitor<DataManagerService>;
  friend class detail::DataManagerSendPutRequestVisitor<DataManagerService>;
  friend class detail::DataManagerDeleteVisitor<DataManagerService>;
  friend class detail::DataManagerSendDeleteVisitor<DataManagerService>;
  friend class detail::PutResponseFailureVisitor<DataManagerService>;
  friend class detail::DataManagerAccountQueryVisitor<DataManagerService>;
  friend class detail::DataManagerGetForReplicationVisitor<DataManagerService>;
  friend class test::DataManagerServiceTest;

  routing::Routing& routing_;
  AsioService asio_service_;
  nfs_client::DataGetter& data_getter_;
  mutable std::mutex accumulator_mutex_, close_nodes_change_mutex_;
  bool stopped_;
  Accumulator<Messages> accumulator_;
  routing::CloseNodesChange close_nodes_change_;
  DataManagerDispatcher dispatcher_;
  routing::Timer<std::pair<PmidName, GetResponseContents>> get_timer_;
  DataManagerDataBase db_;
  Sync<DataManager::UnresolvedPut> sync_puts_;
  Sync<DataManager::UnresolvedDelete> sync_deletes_;
  Sync<DataManager::UnresolvedAddPmid> sync_add_pmids_;
  Sync<DataManager::UnresolvedRemovePmid> sync_remove_pmids_;
  AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kDataManager>> account_transfer_;
  MemoryFIFO temp_store_;

 protected:
  std::mutex lock_guard;
};

// =========================== Handle Message Specialisations ======================================
template <typename MessageType>
void DataManagerService::HandleMessage(const MessageType& /*message*/,
                                       const typename MessageType::Sender& /*sender*/,
                                       const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_genereic_handler_is_available__Specialisation_is_required;
}

template <>
void DataManagerService::HandleMessage(
    const PutRequestFromMaidManagerToDataManager& message,
    const typename PutRequestFromMaidManagerToDataManager::Sender&,
    const typename PutRequestFromMaidManagerToDataManager::Receiver&);

template <>
void DataManagerService::HandleMessage(
    const PutResponseFromPmidManagerToDataManager& message,
    const typename PutResponseFromPmidManagerToDataManager::Sender&,
    const typename PutResponseFromPmidManagerToDataManager::Receiver&);

template <>
void DataManagerService::HandleMessage(
    const PutFailureFromPmidManagerToDataManager& message,
    const typename PutFailureFromPmidManagerToDataManager::Sender& sender,
    const typename PutFailureFromPmidManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

// Special case for relay messages
template <>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodePartialToDataManager& message,
    const typename nfs::GetRequestFromMaidNodePartialToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodePartialToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);


// Special case for relay messages
template <>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterPartialToDataManager& message,
    const typename nfs::GetRequestFromDataGetterPartialToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterPartialToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const GetResponseFromPmidNodeToDataManager& message,
    const typename GetResponseFromPmidNodeToDataManager::Sender& sender,
    const typename GetResponseFromPmidNodeToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const DeleteRequestFromMaidManagerToDataManager& message,
    const typename DeleteRequestFromMaidManagerToDataManager::Sender& sender,
    const typename DeleteRequestFromMaidManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const SynchroniseFromDataManagerToDataManager& message,
    const typename SynchroniseFromDataManagerToDataManager::Sender& sender,
    const typename SynchroniseFromDataManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const AccountTransferFromDataManagerToDataManager& message,
    const typename AccountTransferFromDataManagerToDataManager::Sender& sender,
    const typename AccountTransferFromDataManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const AccountQueryFromDataManagerToDataManager& message,
    const typename AccountQueryFromDataManagerToDataManager::Sender& sender,
    const typename AccountQueryFromDataManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const AccountQueryResponseFromDataManagerToDataManager& message,
    const typename AccountQueryResponseFromDataManagerToDataManager::Sender& sender,
    const typename AccountQueryResponseFromDataManagerToDataManager::Receiver& receiver);

// ================================== Put implementation ===========================================
template <typename Data>
void DataManagerService::HandlePut(const Data& data, const MaidName& maid_name,
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
                << " from maid_node " << HexSubstr(maid_name->string());
  uint64_t cost(static_cast<uint64_t>(data.Serialise().data.string().size()));
  if (!EntryExist<Data>(data.name())) {
    cost *= routing::Parameters::group_size;
    try {
      LOG(kVerbose) << "Store in temp memeory";
      auto serialised_data(data.Serialise().data);
      temp_store_.Store(GetDataNameVariant(Data::Tag::kValue, data.name().value),
                               serialised_data);
      DoSync(DataManager::UnresolvedPut(DataManager::Key(data.name()),
                                        ActionDataManagerPut(serialised_data.string().size(),
                                                             message_id),
                                        routing_.kNodeId()));
      dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id);
    }
    catch (const std::exception&) {
      LOG(kError) << "Failed to store data in to the cache";
    }
  } else {
    HandlePutWhereEntryExists(data, maid_name, message_id, cost, is_unique_on_network<Data>());
  }
}

template <typename Data>
void DataManagerService::HandleSendPutRequest(
    const PmidName& pmid_name, const Data& data, nfs::MessageId message_id) {
  dispatcher_.SendPutRequest(pmid_name, data, message_id);
}

template <typename Data>
bool DataManagerService::EntryExist(const typename Data::Name& name) {
  try {
    db_.Get(DataManager::Key(name.value, Data::Tag::kValue));
    LOG(kInfo) << "Entry does not exist";
    return true;
  }
  catch (const maidsafe_error& /*error*/) {
    LOG(kInfo) << "Entry does not exist";
    return false;
  }
  catch (...) {
    assert(0 && "DataManagerService::EntryExist");
    return false;
  }
}

template <typename Data>
void DataManagerService::HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                                   nfs::MessageId message_id, uint64_t /*cost*/,
                                                   EntryMustBeUnique) {
  LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
              << " from maid_node " << HexSubstr(maid_name->string())
              << " . SendPutFailure with message_id " << message_id.data;
  dispatcher_.SendPutFailure<Data>(maid_name, data.name(),
                                   maidsafe_error(VaultErrors::unique_data_clash), message_id);
}

template <typename Data>
void DataManagerService::HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                                   nfs::MessageId message_id, uint64_t cost,
                                                   EntryNeedNotBeUnique) {
  // As the subscribers has been removed, no need to carry out any further actions for duplicates
  LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
              << " from maid_node " << HexSubstr(maid_name->string())
              << " . SendPutResponse with message_id " << message_id.data;
  dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id);
}

template <typename Data>
void DataManagerService::HandlePutResponse(const typename Data::Name& data_name,
                                           const PmidName& pmid_node,
                                           nfs::MessageId /*message_id*/) {
  LOG(kVerbose) << "DataManagerService::HandlePutResponse for chunk "
                << HexSubstr(data_name.value.string()) << " storing on pmid_node "
                << HexSubstr(pmid_node.value.string());
  typename DataManager::Key key(data_name.value, Data::Tag::kValue);
  DoSync(DataManager::UnresolvedAddPmid(key, ActionDataManagerAddPmid(pmid_node),
         routing_.kNodeId()));
  // if storages nodes reached cap, the existing furthest offline node need to be removed
  DataManager::Value value;
  try {
    value = db_.Get(key);
  }
  catch (const maidsafe_error& error) {
    if (error.code() == make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "DataManagerService::HandlePutResponse value does not exist..."
                  << boost::diagnostic_information(error);
      return;
    }
    throw;
  }
  PmidName pmid_node_to_remove;
  auto need_to_prune(false);
  {
    std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
    need_to_prune = value.NeedToPrune(close_nodes_change_.new_close_nodes(), pmid_node_to_remove);
  }
  if (need_to_prune)
    DoSync(DataManager::UnresolvedRemovePmid(key,
               ActionDataManagerRemovePmid(pmid_node_to_remove), routing_.kNodeId()));
}

template <typename Data>
void DataManagerService::HandlePutFailure(const typename Data::Name& data_name,
                                          uint64_t size,
                                          const PmidName& attempted_pmid_node,
                                          nfs::MessageId message_id,
                                          const maidsafe_error& /*error*/) {
  LOG(kVerbose) << "DataManagerService::HandlePutFailure " << HexSubstr(data_name.value)
                << " from attempted_pmid_node " << HexSubstr(attempted_pmid_node->string());
  // TODO(Team): Following should be done only if error is fixable by repeat
  uint64_t chunk_size(0);
  typename DataManager::Key key(data_name.value, Data::Tag::kValue);
  if (SendPutRetryRequired(data_name))
    chunk_size = Replicate(key, message_id, attempted_pmid_node);

  DoSync(DataManager::UnresolvedRemovePmid(
      key, ActionDataManagerRemovePmid(attempted_pmid_node), routing_.kNodeId()));

  if (chunk_size != 0 && chunk_size != size)
    SendPmidUpdateAccount<Data>(data_name, attempted_pmid_node, chunk_size, size);

  DerankPmidNode(attempted_pmid_node);
}

template <typename DataName>
bool DataManagerService::SendPutRetryRequired(const DataName& data_name) {
  try {
    // mutex is required
    auto value(db_.Get(DataManager::Key(data_name.value, DataName::data_type::Tag::kValue)));
    return value.AllPmids().size() < routing::Parameters::group_size;
  }
  catch (const maidsafe_error& /*error*/) {}
  return false;
}

// ==================== Get / IntegrityCheck implementation ========================================
template <typename Data, typename RequestorIdType>
void DataManagerService::HandleGet(const typename Data::Name& data_name,
                                   const RequestorIdType& requestor,
                                   nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManagerService::HandleGet " << HexSubstr(data_name.value);
  try {
    dispatcher_.SendGetResponseSuccess(
        requestor,
        Data(data_name,
             typename Data::serialised_type(temp_store_.Get(DataNameVariant(data_name)))),
        message_id);
    return;
  }
  catch (const maidsafe_error& /*error*/) {
    LOG(kVerbose) << "data not available in temporary store";
  }

  // Get all pmid nodes that are online.
  std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
  int expected_response_count(static_cast<int>(online_pmids.size()));
  // if there is no online_pmids in record, means :
  //   this DM doesn't have the record for the requested data
  //   or no pmid can given the data (shall not happen)
  // BEFORE_RELEASE In any case, shall return silently or send back a failure?
  if (expected_response_count == 0) {
    dispatcher_.SendGetResponseFailure(requestor, data_name,
                                       maidsafe_error(CommonErrors::no_such_element), message_id);
    return;
  }

  // Choose the one we're going to ask for actual data, and set up the others for integrity checks.
  auto pmid_node_to_get_from(ChoosePmidNodeToGetFrom(online_pmids, data_name));
  std::map<PmidName, IntegrityCheckData> integrity_checks;
  // TODO(Team): IntegrityCheck is temporarily disabled because of the performance concern
  //             1, May only undertake IntegrityCheck for mutable data
  //             2, The efficiency of the procedure shall be improved
//   for (const auto& iter : online_pmids)
//     integrity_checks.insert(
//         std::make_pair(iter, IntegrityCheckData(IntegrityCheckData::GetRandomInput())));

  // Create helper struct which holds the collection of responses, and add the task to the timer.
  auto get_response_op(
      std::make_shared<detail::GetResponseOp<typename Data::Name, RequestorIdType>>(
          pmid_node_to_get_from, message_id, integrity_checks, data_name, requestor));
  auto functor([=](const std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    LOG(kVerbose) << "DataManagerService::HandleGet " << HexSubstr(data_name.value)
                  << " task called from timer to DoHandleGetResponse";
    this->DoHandleGetResponse<Data, RequestorIdType>(pmid_node_and_contents.first,
                                                     pmid_node_and_contents.second,
                                                     get_response_op);
  });
  get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, 1/*expected_response_count*/,
                     message_id.data);
  LOG(kVerbose) << "DataManagerService::HandleGet " << HexSubstr(data_name.value)
                << " SendGetRequest with message_id " << message_id.data
                << " to picked up pmid_node " << HexSubstr(pmid_node_to_get_from->string());
  // Send requests
  dispatcher_.SendGetRequest<Data>(pmid_node_to_get_from, data_name, message_id);

//   LOG(kVerbose) << "DataManagerService::HandleGet " << HexSubstr(data_name.value)
//                 << " has " << integrity_checks.size() << " entries to check integrity";

//   for (const auto& integrity_check : integrity_checks) {
//     dispatcher_.SendIntegrityCheck<Data>(data_name,
//                                          NonEmptyString(integrity_check.second.random_input()),
//                                          integrity_check.first, message_id);
//   }
}

template <typename Data>
void DataManagerService::GetForReplication(const PmidName& pmid_name,
                                           const typename Data::Name& data_name) {
  std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
  online_pmids.erase(pmid_name);
  DoGetForReplication<Data>(data_name, online_pmids);
}

template <typename Data>
void DataManagerService::DoGetForReplication(const typename Data::Name& data_name,
                                             const std::set<PmidName>& online_pmids) {
  LOG(kVerbose) << "DataManagerService::GetForNodeDown chunk " << HexSubstr(data_name.value);
  // Just get, don't do integrity check
  auto functor([=](const std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    LOG(kVerbose) << "DataManagerService::GetForNodeDown " << HexSubstr(data_name.value)
                  << " task called from timer to DoGetForNodeDownResponse";
    this->DoGetResponseForReplication<Data>(pmid_node_and_contents.first, data_name,
                                            pmid_node_and_contents.second);
  });
  nfs::MessageId message_id(get_timer_.NewTaskId());
  get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, 1, message_id);
  for (auto& pmid_node : online_pmids) {
    LOG(kVerbose) << "DataManagerService::GetForNodeDown " << HexSubstr(data_name.value)
                  << " SendGetRequest with message_id " << message_id.data
                  << " to picked up pmid_node " << HexSubstr(pmid_node->string());
    dispatcher_.SendGetRequest<Data>(pmid_node, data_name, message_id);
  }
}

template <typename DataName>
PmidName DataManagerService::ChoosePmidNodeToGetFrom(std::set<PmidName>& online_pmids,
                                                     const DataName& data_name) const {
  LOG(kVerbose) << "ChoosePmidNodeToGetFrom having following online_pmids : ";
  for (auto pmid : online_pmids)
    LOG(kVerbose) << "       online_pmids       ---     " << HexSubstr(pmid->string());
  // Convert the set of PmidNames to a set of NodeIds
  std::set<NodeId> online_node_ids;
  auto hint_itr(std::end(online_node_ids));
  std::for_each(std::begin(online_pmids), std::end(online_pmids),
                [&](const PmidName& name) {
                  hint_itr = online_node_ids.insert(hint_itr, NodeId(name->string()));
                });

  PmidName chosen;
  {
    std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
//     LOG(kVerbose) << "ChoosePmidNodeToGetFrom matrix containing following info : ";
//     close_nodes_change_.Print();
    chosen = PmidName(Identity(
        close_nodes_change_.ChoosePmidNode(online_node_ids, NodeId(data_name->string())).string()));
  }

  online_pmids.erase(chosen);
  LOG(kVerbose) << "PmidNode : " << HexSubstr(chosen->string()) << " is chosen by this DataManager";
  return chosen;
}

template <typename Data>
std::set<PmidName> DataManagerService::GetOnlinePmids(const typename Data::Name& data_name) {
  std::set<PmidName> online_pmids_set;
  try {
    auto value(db_.Get(DataManager::Key(data_name.value, Data::Tag::kValue)));
    std::lock_guard<std::mutex> lock(close_nodes_change_mutex_);
    auto online_pmids(value.online_pmids(close_nodes_change_.new_close_nodes()));
    for (auto online_pmid : online_pmids)
      online_pmids_set.insert(online_pmid);
  } catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "DataManagerService::GetOnlinePmids encountered unknown error "
                  << boost::diagnostic_information(error);
      throw;  // For db errors
    }
    // TODO(Fraser#5#): 2013-10-03 - Request for non-existent data should possibly generate an alert
    LOG(kWarning) << "Entry for " << HexSubstr(data_name.value) << " doesn't exist.";
//     throw VaultErrors::no_such_account;
  }
  return online_pmids_set;
}


template <typename Data, typename RequestorIdType>
void DataManagerService::DoHandleGetResponse(
    const PmidName& pmid_node, const GetResponseContents& contents,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  // Note: if 'pmid_node' and 'contents' is default-constructed, it's probably a result of this
  // function being invoked by the timer after timeout.
  int called_count(0), expected_count(0);
  {
    std::lock_guard<std::mutex> lock(get_response_op->mutex);
    if (contents.content && pmid_node.value.IsInitialised())
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse received response from "
                    << HexSubstr(pmid_node->string()) << " for chunk "
                    << HexSubstr(contents.name.raw_name) << " with content "
                    << HexSubstr(contents.content->string());

    called_count = ++get_response_op->called_count;
    expected_count = static_cast<int>(get_response_op->integrity_checks.size()) + 1;
    assert(called_count <= expected_count);
    if (pmid_node == get_response_op->pmid_node_to_get_from) {
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse send response to requester";
      if (contents.content)
        if (SendGetResponse<Data, RequestorIdType>(
                Data(get_response_op->data_name, typename Data::serialised_type(*contents.content)),
                get_response_op)) {
        get_response_op->serialised_contents = typename Data::serialised_type(*contents.content);
      }
    } else if (contents.check_result) {
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse set integrity check_result "
                    << "for pmid_node " << HexSubstr(pmid_node->string());
      auto itr(get_response_op->integrity_checks.find(pmid_node));
      if (itr != std::end(get_response_op->integrity_checks))
        itr->second.SetResult(*contents.check_result);
    } else {
      // In case of timer timeout, the pmid_node and contents will be constructed using default.
      LOG(kWarning) << "DataManagerService::DoHandleGetResponse reached timed out branch";
      AssessGetContentRequestedPmidNode<Data, RequestorIdType>(get_response_op);
    }
  }
  LOG(kVerbose) << "DataManagerService::DoHandleGetResponse called_count "
                << called_count << " expected_count " << expected_count;
  if (called_count == expected_count)
    AssessGetContentRequestedPmidNode<Data, RequestorIdType>(get_response_op);
}

template <typename Data>
void DataManagerService::DoGetResponseForReplication(const PmidName& pmid_node,
                                                     const typename Data::Name& data_name,
                                                     const GetResponseContents& contents) {
  // Note: if 'pmid_node' and 'contents' is default-constructed, it's probably a result of this
  // function being invoked by the timer after timeout.
  LOG(kVerbose) << "DataManagerService::DoGetForReplicationResponse "
                << HexSubstr(data_name->string());
  {
    std::lock_guard<std::mutex> lock(this->close_nodes_change_mutex_);
    if (this->stopped_)
      return;
  }

  if (contents.content && pmid_node.value.IsInitialised())
    LOG(kVerbose) << "DataManagerService::DoGetForReplicationResponse received response from "
                  << HexSubstr(pmid_node->string()) << " for chunk "
                  << HexSubstr(contents.name.raw_name) << " with content "
                  << HexSubstr(contents.content->string());

  if (contents.content) {
    temp_store_.Store(GetDataNameVariant(Data::Tag::kValue, data_name.value),
                      typename Data::serialised_type(*contents.content));
    Replicate(DataManager::Key(data_name.value, Data::Tag::kValue), nfs::MessageId(RandomInt32()));
  }
}

template <typename Data, typename RequestorIdType>
bool DataManagerService::SendGetResponse(
    const Data& data,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  maidsafe_error error(MakeError(CommonErrors::unknown));
  try {
    LOG(kInfo) << "DataManagerService::SendGetResponse SendGetResponseSuccess and put to cache"
               << get_response_op->message_id;
    dispatcher_.SendGetResponseSuccess(get_response_op->requestor_id, data,
                                       get_response_op->message_id);
    temp_store_.Store(GetDataNameVariant(Data::Tag::kValue, data.name().value),
                      data.Serialise().data);
    return true;
  } catch(const maidsafe_error& e) {
    error = e;
    LOG(kError) << "DataManagerService::SendGetResponse " << boost::diagnostic_information(e);
  } catch(const std::exception& e) {
    LOG(kError) << "DataManagerService::SendGetResponse " << boost::diagnostic_information(e);
  } catch(...) {
    LOG(kError) << "DataManagerService::SendGetResponse Unexpected exception type.";
  }
  LOG(kWarning) << "DataManagerService::SendGetResponse SendGetResponseFailure";
  dispatcher_.SendGetResponseFailure(get_response_op->requestor_id, get_response_op->data_name,
                                     error, get_response_op->message_id);
  return false;
}

template <typename Data, typename RequestorIdType>
void DataManagerService::AssessGetContentRequestedPmidNode(
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  // If we failed to get the serialised_contents, mark 'pmid_node_to_get_from' as down, sync this,
  // try to Get from peer DataManagers' caches.
  if (!get_response_op->serialised_contents->IsInitialised()) {
    LOG(kWarning) << "DataManagerService::AssessGetContentRequestedPmidNode marking node "
                  << HexSubstr(get_response_op->pmid_node_to_get_from->string())
                  << " down for data " << HexSubstr(get_response_op->data_name.value.string());
    MarkNodeDown(get_response_op->pmid_node_to_get_from, get_response_op->data_name);
//     auto functor([=](const GetCachedResponseContents& contents) {
//       DoHandleGetCachedResponse<Data, RequestorIdType>(contents, get_response_op);
//     });
//     // BEFORE_RELEASE The following line of code will try to AddTask with same task_id
//     //                if marking two_nodes down at the same time, which will fail the assertion
//     get_cached_response_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, 1,
//                                        get_response_op->message_id);
//     dispatcher_.SendGetFromCache(get_response_op->data_name);
  } else {
    AssessIntegrityCheckResults<Data, RequestorIdType>(get_response_op);
  }
}

template <typename Data, typename RequestorIdType>
void DataManagerService::AssessIntegrityCheckResults(
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  try {
    for (const auto& itr : get_response_op->integrity_checks) {
      if (!itr.second.Validate(get_response_op->serialised_contents)) {
        // In case the pmid_node_to_get_from returned with a false content, which in-validated
        // all the others, as the PmidNode side has to accumulate enough delete requests before
        // deploy the action, send out false delete request won't cause problem as long as no more
        // than half PmidNodes containing false data.
        if (itr.second.result() != IntegrityCheckData::Result()) {
          LOG(kWarning) << "DataManagerService::AssessIntegrityCheckResults detected pmid_node "
                        << HexSubstr(itr.first->string()) << " returned invalid data for "
                        << HexSubstr(get_response_op->data_name.value.string());
          DerankPmidNode(itr.first);
          DeletePmidNodeAsHolder<Data>(itr.first, get_response_op->data_name,
                                      get_response_op->message_id);
          typename DataManager::Key key(get_response_op->data_name.value, Data::Tag::kValue);
          SendFalseDataNotification<Data>(itr.first, get_response_op->data_name,
                                          db_.Get(key).chunk_size(),
                                          get_response_op->message_id);
        } else {
          LOG(kWarning) << "DataManagerService::AssessIntegrityCheckResults detected pmid_node "
                        << HexSubstr(itr.first->string()) << " holding data "
                        << HexSubstr(get_response_op->data_name.value.string()) << " is down";
          MarkNodeDown(itr.first, get_response_op->data_name);
        }
      }
    }
  } catch(...) {
    LOG(kWarning) << "caught exception in DataManagerService::AssessIntegrityCheckResults";
  }
}

template <typename Data>
void DataManagerService::DeletePmidNodeAsHolder(const PmidName pmid_node,
                                                const typename Data::Name& name,
                                                nfs::MessageId message_id) {
  typename DataManager::Key key(name.value, Data::Tag::kValue);
  DoSync(DataManager::UnresolvedRemovePmid(key,
             ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId()));
  SendDeleteRequest<Data>(pmid_node, name, db_.Get(key).chunk_size(), message_id);
}

// =================== Delete implementation ======================================================
template <typename Data>
void DataManagerService::HandleDelete(const typename Data::Name& data_name,
                                      nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManagerService::HandleDelete " << HexSubstr(data_name.value);
  typename DataManager::Key key(data_name.value, Data::Name::data_type::Tag::kValue);
  DoSync(DataManager::UnresolvedDelete(key, ActionDataManagerDelete(message_id),
                                       routing_.kNodeId()));
}

template <typename Data>
void DataManagerService::SendDeleteRequest(const PmidName pmid_node,
    const typename Data::Name& name, const uint64_t size, nfs::MessageId message_id) {
  dispatcher_.SendDeleteRequest<Data>(pmid_node, name, size, message_id);
}

template <typename Data>
void DataManagerService::SendFalseDataNotification(const PmidName pmid_node,
    const typename Data::Name& name, uint64_t size, nfs::MessageId message_id) {
  dispatcher_.SendFalseDataNotification<Data>(pmid_node, name, size, message_id);
}

// ==================== Node up / Node down implementation =========================================
template <typename DataName>
void DataManagerService::MarkNodeDown(const PmidName& pmid_node, const DataName& name) {
  LOG(kWarning) << "DataManager marking node " << HexSubstr(pmid_node->string())
                << " down for chunk " << HexSubstr(name.value.string());
  Replicate(DataManager::Key(name), nfs::MessageId(RandomInt32()), pmid_node);
}

// ==================== Sync =======================================================================

template <typename UnresolvedAction>
void DataManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_add_pmids_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_pmids_, unresolved_action);
}

template<typename DataName>
void DataManagerService::HandleAccountQuery(const DataName& name, const NodeId& sender) {
  if (!close_nodes_change_.CheckIsHolder(NodeId(name->string()), sender)) {
    LOG(kWarning) << "attempt to obtain account from non-holder";
    return;
  }
  try {
    auto value(db_.Get(DataManager::Key(name)));
    protobuf::AccountTransfer account_transfer_proto;
    protobuf::DataManagerKeyValuePair kv_msg;
    kv_msg.set_key(DataManager::Key(name).Serialise());
    kv_msg.set_value(value.Serialise());
    account_transfer_proto.add_serialised_accounts(kv_msg.SerializeAsString());
    dispatcher_.SendAccountResponse(account_transfer_proto.SerializeAsString(),
                                    routing::GroupId(NodeId(name->string())), sender);
  }
  catch (const std::exception& error) {
    LOG(kError) << "failed to retrieve account: " << error.what();
  }
}

template <typename Data>
void DataManagerService::SendPmidUpdateAccount(
    const typename Data::Name& data_name, const PmidName& pmid_node, uint64_t chunk_size,
    uint64_t given_size) {
  dispatcher_.SendPmidUpdateAccount<Data>(data_name, pmid_node, chunk_size, given_size);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
