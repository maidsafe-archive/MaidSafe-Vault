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

#include "maidsafe/vault/account_transfer.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/operation_visitors.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/data_manager/dispatcher.h"
#include "maidsafe/vault/data_manager/helpers.h"
#include "maidsafe/vault/data_manager/value.h"

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
  void HandlePut(const Data& data, const MaidName& maid_name, const PmidName& pmid_name,
                 nfs::MessageId message_id);

  template <typename Data>
  bool EntryExist(const typename Data::Name& name);

  typedef std::true_type EntryMustBeUnique;
  typedef std::false_type EntryNeedNotBeUnique;
  template <typename Data>
  void HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                 nfs::MessageId message_id, int32_t cost, EntryMustBeUnique);
  template <typename Data>
  void HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                 nfs::MessageId message_id, int32_t cost, EntryNeedNotBeUnique);

  template <typename Data>
  void HandlePutResponse(const typename Data::Name& data_name, const PmidName& pmid_node,
                         int32_t size, nfs::MessageId message_id);

  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data_name, const PmidName& attempted_pmid_node,
                        nfs::MessageId message_id, const maidsafe_error& error);

  template <typename DataName>
  bool SendPutRetryRequired(const DataName& data_name);

  // =========================== Get section (includes integrity checks) ===========================
  typedef GetResponseFromPmidNodeToDataManager::Contents GetResponseContents;
  typedef GetCachedResponseFromCacheHandlerToDataManager::Contents GetCachedResponseContents;

  template <typename Data, typename RequestorIdType>
  void HandleGet(const typename Data::Name& data_name, const RequestorIdType& requestor,
                 nfs::MessageId message_id);

  template <typename Data>
  void GetForNodeDown(const PmidName& pmid_name, const typename Data::Name& data_name);

  void HandleGetResponse(const nfs_vault::DataNameAndContentOrCheckResult& response,
                         nfs::MessageId message_id);

  void HandleGetCachedResponse(nfs::MessageId message_id,
                               const GetCachedResponseContents& contents);

  // Removes a pmid_name from the set and returns it.
  template <typename DataName>
  PmidName ChoosePmidNodeToGetFrom(std::set<PmidName>& online_pmids,
                                   const DataName& data_name) const;
  template <typename Data>
  std::set<PmidName> GetOnlinePmids(const typename Data::Name& data_name);

  template <typename Data, typename RequestorIdType>
  void DoHandleGetResponse(
    nfs_vault::DataNameAndContentOrCheckResult response,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data, typename RequestorIdType>
  void DoHandleGetCachedResponse(
      const GetCachedResponseContents& contents,
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data>
  void DoGetForNodeDownResponse(const PmidName& pmid_name, const typename Data::Name& data_name,
                                const nfs_vault::DataNameAndContentOrCheckResult& response);

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

  template <typename Data>
  void DerankPmidNode(const PmidName pmid_node, const typename Data::Name& name,
                      nfs::MessageId message_id);

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
                         nfs::MessageId message_id);

  template <typename Data>
  void SendFalseDataNotification(const PmidName pmid_node, const typename Data::Name& name,
                                 nfs::MessageId message_id);

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

  void HandleAccountTransfer(
      std::unique_ptr<DataManager::UnresolvedAccountTransfer>&& resolved_action);
  // =========================== General functions =================================================
  void HandleDataIntegrityResponse(const GetResponseContents& response, nfs::MessageId message_id);

  template <typename Data>
  NonEmptyString GetContentFromCache(const typename Data::Name& /*name*/) {
    // BEFORE_RELEASE implementation missing
    return NonEmptyString("remove me");
  }

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
  friend class detail::DataManagerDeleteVisitor<DataManagerService>;
  friend class detail::DataManagerSendDeleteVisitor<DataManagerService>;
  friend class detail::PutResponseFailureVisitor<DataManagerService>;
  friend class detail::DataManagerSetPmidOnlineVisitor<DataManagerService>;
  friend class detail::DataManagerSetPmidOfflineVisitor<DataManagerService>;
  friend class test::DataManagerServiceTest;

  routing::Routing& routing_;
  AsioService asio_service_;
  nfs_client::DataGetter& data_getter_;
  mutable std::mutex accumulator_mutex_, close_nodes_change_mutex_;
  bool stopped_;
  Accumulator<Messages> accumulator_;
  routing::CloseNodesChange close_nodes_change_;
  DataManagerDispatcher dispatcher_;
  routing::Timer<nfs_vault::DataNameAndContentOrCheckResult> get_timer_;
  routing::Timer<GetCachedResponseContents> get_cached_response_timer_;
  Db<DataManager::Key, DataManager::Value> db_;
  Sync<DataManager::UnresolvedPut> sync_puts_;
  Sync<DataManager::UnresolvedDelete> sync_deletes_;
  Sync<DataManager::UnresolvedAddPmid> sync_add_pmids_;
  Sync<DataManager::UnresolvedRemovePmid> sync_remove_pmids_;
  Sync<DataManager::UnresolvedNodeDown> sync_node_downs_;
  Sync<DataManager::UnresolvedNodeUp> sync_node_ups_;
  AccountTransfer<DataManager::UnresolvedAccountTransfer> account_transfer_;

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
    const GetCachedResponseFromCacheHandlerToDataManager& message,
    const typename GetCachedResponseFromCacheHandlerToDataManager::Sender& sender,
    const typename GetCachedResponseFromCacheHandlerToDataManager::Receiver& receiver);

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
  const SetPmidOnlineFromPmidManagerToDataManager& message,
    const typename SetPmidOnlineFromPmidManagerToDataManager::Sender& sender,
    const typename SetPmidOnlineFromPmidManagerToDataManager::Receiver& receiver);

template <>
void DataManagerService::HandleMessage(
    const SetPmidOfflineFromPmidManagerToDataManager& message,
    const typename SetPmidOfflineFromPmidManagerToDataManager::Sender& sender,
    const typename SetPmidOfflineFromPmidManagerToDataManager::Receiver& receiver);

// ================================== Put implementation ===========================================
template <typename Data>
void DataManagerService::HandlePut(const Data& data, const MaidName& maid_name,
                                   const PmidName& pmid_name_in, nfs::MessageId message_id) {
  LOG(kVerbose) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
                << " from maid_node " << HexSubstr(maid_name->string())
                << " with pmid_name_in " << HexSubstr(pmid_name_in->string());
  int32_t cost(static_cast<int32_t>(data.Serialise().data.string().size()));
  if (!EntryExist<Data>(data.name())) {
    cost *= routing::Parameters::group_size;
    PmidName pmid_name;
    if (routing_.ClosestToId(NodeId(data.name().value)) &&
        (pmid_name_in.value.string() != NodeId().string()) &&
        (pmid_name_in.value.string() != data.name().value.string())) {
      LOG(kInfo) << "using the pmid_name_in";
      pmid_name = pmid_name_in;
    } else {
      do {
        LOG(kInfo) << "pick from routing_.RandomConnectedNode()";
        // it is observed during the startup period, a vault may receive a request before
        // it's routing_table having any entry.
        auto picked_node(routing_.RandomConnectedNode());
        if (picked_node != NodeId()) {
          pmid_name = PmidName(Identity(picked_node.string()));
        } else {
          LOG(kError) << "no entry in routing_table";
          return;
        }
      } while (pmid_name->string() == data.name().value.string());
    }
    LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
               << " from maid_node " << HexSubstr(maid_name->string())
               << " . SendPutRequest with message_id " << message_id.data
               << " to picked up pmid_node " << HexSubstr(pmid_name->string());
    dispatcher_.SendPutRequest(pmid_name, data, message_id);
    LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
                << " from maid_node " << HexSubstr(maid_name->string())
                << " . SendPutResponse with message_id " << message_id.data;
    dispatcher_.SendPutToCache(data);
    dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id);
  } else {
    HandlePutWhereEntryExists(data, maid_name, message_id, cost, is_unique_on_network<Data>());
  }
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
                                                   nfs::MessageId message_id, int32_t /*cost*/,
                                                   EntryMustBeUnique) {
  LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
              << " from maid_node " << HexSubstr(maid_name->string())
              << " . SendPutFailure with message_id " << message_id.data;
  dispatcher_.SendPutFailure<Data>(maid_name, data.name(),
                                   maidsafe_error(VaultErrors::unique_data_clash), message_id);
}

template <typename Data>
void DataManagerService::HandlePutWhereEntryExists(const Data& data, const MaidName& maid_name,
                                                   nfs::MessageId message_id, int32_t cost,
                                                   EntryNeedNotBeUnique) {
  LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
              << " from maid_node " << HexSubstr(maid_name->string()) << " syncing";
  typename DataManager::Key key(data.name().value, Data::Tag::kValue);
  DoSync(DataManager::UnresolvedPut(key, ActionDataManagerPut(), routing_.kNodeId()));
  LOG(kInfo) << "DataManagerService::HandlePut " << HexSubstr(data.name().value)
              << " from maid_node " << HexSubstr(maid_name->string())
              << " . SendPutResponse with message_id " << message_id.data;
  dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id);
}

template <typename Data>
void DataManagerService::HandlePutResponse(const typename Data::Name& data_name,
                                           const PmidName& pmid_node, int32_t size,
                                           nfs::MessageId /*message_id*/) {
  LOG(kVerbose) << "DataManagerService::HandlePutResponse for chunk "
                << HexSubstr(data_name.value.string()) << " storing on pmid_node "
                << HexSubstr(pmid_node.value.string());
  typename DataManager::Key key(data_name.value, Data::Tag::kValue);
  DoSync(DataManager::UnresolvedAddPmid(key,
             ActionDataManagerAddPmid(pmid_node, size), routing_.kNodeId()));
}

template <typename Data>
void DataManagerService::HandlePutFailure(const typename Data::Name& data_name,
                                          const PmidName& attempted_pmid_node,
                                          nfs::MessageId message_id,
                                          const maidsafe_error& /*error*/) {
  LOG(kVerbose) << "DataManagerService::HandlePutFailure " << HexSubstr(data_name.value)
                << " from attempted_pmid_node " << HexSubstr(attempted_pmid_node->string());
  // TODO(Team): Following should be done only if error is fixable by repeat
  typename DataManager::Key key(data_name.value, Data::Tag::kValue);
  // Get all pmid nodes for this data.
  if (SendPutRetryRequired(data_name)) {
    std::set<PmidName> pmids_to_avoid;
    try {
      auto value(db_.Get(key));
      pmids_to_avoid = std::move(value.AllPmids());
    } catch (const common_error& error) {
      if (error.code() != make_error_code(CommonErrors::no_such_element)) {
        LOG(kError) << "HandlePutFailure db error";
        throw error;  // For db errors
      }
    }

    pmids_to_avoid.insert(attempted_pmid_node);
    auto pmid_name(PmidName(Identity(routing_.RandomConnectedNode().string())));
    while (pmids_to_avoid.find(pmid_name) != std::end(pmids_to_avoid))
      pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));

    try {
      NonEmptyString content(GetContentFromCache<Data>(data_name));
      dispatcher_.SendPutRequest<Data>(pmid_name,
                                       Data(data_name, typename Data::serialised_type(content)),
                                       message_id);
    }
    catch (std::exception& /*ex*/) {
      // handle failure to retrieve content from cache, a Get->Then->call
      // dispatcher_.SendPutRequest(pmid_name, Data(data_name, content), message_id); )
    }
  }

  DoSync(DataManager::UnresolvedRemovePmid(
      key, ActionDataManagerRemovePmid(attempted_pmid_node), routing_.kNodeId()));
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
  auto functor([=](const nfs_vault::DataNameAndContentOrCheckResult& response) {  // std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    LOG(kVerbose) << "DataManagerService::HandleGet " << HexSubstr(data_name.value)
                  << " task called from timer to DoHandleGetResponse";
    this->DoHandleGetResponse<Data, RequestorIdType>(response, get_response_op);
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
void DataManagerService::GetForNodeDown(const PmidName& pmid_name,
                                        const typename Data::Name& data_name) {
  LOG(kVerbose) << "DataManagerService::GetForNodeDown chunk " << HexSubstr(data_name.value);
  std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
  online_pmids.erase(pmid_name);
  // Only trigger the recovery procedure when not enough online_pmids.
  if (online_pmids.size() > (routing::Parameters::group_size / 2U))
    return;
  // Just get, don't do integrity check
  auto functor([=](const nfs_vault::DataNameAndContentOrCheckResult& response) {
    LOG(kVerbose) << "DataManagerService::GetForNodeDown " << HexSubstr(data_name.value)
                  << " task called from timer to DoGetForNodeDownResponse";
    this->DoGetForNodeDownResponse<Data>(pmid_name, data_name, response);
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
  std::set<PmidName> online_pmids;
  try {
    auto value(db_.Get(DataManager::Key(data_name.value, Data::Tag::kValue)));
    online_pmids = std::move(value.online_pmids());
  } catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "DataManagerService::GetOnlinePmids encountered unknown error "
                  << boost::diagnostic_information(error);
      throw error;  // For db errors
    }
    // TODO(Fraser#5#): 2013-10-03 - Request for non-existent data should possibly generate an alert
    LOG(kWarning) << "Entry for " << HexSubstr(data_name.value) << " doesn't exist.";
//     throw VaultErrors::no_such_account;
  }
  return online_pmids;
}


template <typename Data, typename RequestorIdType>
void DataManagerService::DoHandleGetResponse(
    nfs_vault::DataNameAndContentOrCheckResult response,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  // Note: if 'pmid_node' and 'contents' is default-constructed, it's probably a result of this
  // function being invoked by the timer after timeout.

  int called_count(0), expected_count(0);
  {
    std::lock_guard<std::mutex> lock(get_response_op->mutex);
    if (response.content && get_response_op->pmid_node_to_get_from.value.IsInitialised())
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse received response from "
                    << HexSubstr(get_response_op->pmid_node_to_get_from->string()) << " for chunk "
                    << HexSubstr(response.name.raw_name) << " with content "
                    << HexSubstr(response.content->string());

    called_count = ++get_response_op->called_count;
    expected_count = static_cast<int>(get_response_op->integrity_checks.size()) + 1;
    assert(called_count <= expected_count);
    if (response.pmid_name == get_response_op->pmid_node_to_get_from) {
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse send response to requester";
      if (response.content)
        if (SendGetResponse<Data, RequestorIdType>(
                Data(get_response_op->data_name, typename Data::serialised_type(*response.content)),
                get_response_op)) {
          get_response_op->serialised_contents = typename Data::serialised_type(*response.content);
        }
    } else if (response.check_result) {
      LOG(kVerbose) << "DataManagerService::DoHandleGetResponse set integrity check_result "
                    << "for pmid_node " << HexSubstr(response.pmid_name->string());
      auto itr(get_response_op->integrity_checks.find(response.pmid_name));
      if (itr != std::end(get_response_op->integrity_checks))
        itr->second.SetResult(*response.check_result);
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

template <typename Data, typename RequestorIdType>
void DataManagerService::DoHandleGetCachedResponse(
    const GetCachedResponseContents& contents,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  if (contents == GetCachedResponseContents()) {
    LOG(kError) << "Failure to retrieve data from network";
    return;
  }
  if (SendGetResponse<Data, RequestorIdType>(
          Data(get_response_op->data_name,
               typename Data::serialised_type(NonEmptyString(contents.content->data))),
          get_response_op)) {
    get_response_op->serialised_contents =
        typename Data::serialised_type(NonEmptyString(contents.content->data));
    AssessIntegrityCheckResults<Data, RequestorIdType>(get_response_op);
  }
}

template <typename Data>
void DataManagerService::DoGetForNodeDownResponse(const PmidName& pmid_name,
      const typename Data::Name& data_name,
      const nfs_vault::DataNameAndContentOrCheckResult& response) {
  // Note: if 'pmid_node' and 'contents' is default-constructed, it's probably a result of this
  // function being invoked by the timer after timeout.
  LOG(kVerbose) << "DataManagerService::DoGetForNodeDownResponse "
                << HexSubstr(data_name->string());
  {
    std::lock_guard<std::mutex> lock(this->close_nodes_change_mutex_);
    if (this->stopped_)
      return;
  }

  if (response.content && pmid_name.value.IsInitialised())
    LOG(kVerbose) << "DataManagerService::DoGetForNodeDownResponse received response from "
                  << HexSubstr(pmid_name->string()) << " for chunk "
                  << HexSubstr(response.name.raw_name) << " with content "
                  << HexSubstr(response.content->string());

  if (response.content) {
    std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
    PmidName pmid_name;
    bool already_picked(false);
    do {
      pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));
      auto itr(online_pmids.find(pmid_name));
      already_picked = (itr != online_pmids.end());
    } while (pmid_name->string() == data_name.value.string() && already_picked);

    Data data(Data(data_name, typename Data::serialised_type(*response.content)));
    nfs::MessageId message_id(HashStringToMessageId(data_name.value.string()));
    // Moving chunk 'response.name.raw_name' to Pmid node 'pmid_name.value'
    VLOG(nfs::Persona::kDataManager, VisualiserAction::kMoveChunk, response.name.raw_name,
         pmid_name.value);
    dispatcher_.SendPutRequest(pmid_name, data, message_id);
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
    // Put to the CacheHandler in this vault.
    dispatcher_.SendPutToCache(data);
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
          DerankPmidNode<Data>(itr.first, get_response_op->data_name, get_response_op->message_id);
          DeletePmidNodeAsHolder<Data>(itr.first, get_response_op->data_name,
                                      get_response_op->message_id);
          SendFalseDataNotification<Data>(itr.first, get_response_op->data_name,
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
void DataManagerService::DerankPmidNode(const PmidName /*pmid_node*/,
                                        const typename Data::Name& /*name*/,
                                        nfs::MessageId /*message_id*/) {
  // BEFORE_RELEASE: to be implemented
}

template <typename Data>
void DataManagerService::DeletePmidNodeAsHolder(const PmidName pmid_node,
                                                const typename Data::Name& name,
                                                nfs::MessageId message_id) {
  typename DataManager::Key key(name.value, Data::Tag::kValue);
  DoSync(DataManager::UnresolvedRemovePmid(key,
             ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId()));
  SendDeleteRequest<Data>(pmid_node, name, message_id);
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
void DataManagerService::SendDeleteRequest(
    const PmidName pmid_node, const typename Data::Name& name, nfs::MessageId message_id) {
  dispatcher_.SendDeleteRequest<Data>(pmid_node, name, message_id);
}

template <typename Data>
void DataManagerService::SendFalseDataNotification(
    const PmidName pmid_node, const typename Data::Name& name, nfs::MessageId message_id) {
  dispatcher_.SendFalseDataNotification<Data>(pmid_node, name, message_id);
}

// ==================== Node up / Node down implementation =========================================
template <typename DataName>
void DataManagerService::MarkNodeDown(const PmidName& pmid_node, const DataName& name) {
  LOG(kWarning) << "DataManager marking node " << HexSubstr(pmid_node->string())
                << " down for chunk " << HexSubstr(name.value.string());
  typename DataManager::Key key(name.value, DataName::data_type::Tag::kValue);
  DoSync(DataManager::UnresolvedNodeDown(key,
             ActionDataManagerNodeDown(pmid_node), routing_.kNodeId()));
  GetForNodeDown<typename DataName::data_type>(pmid_node, name);
}

template <typename DataName>
void DataManagerService::MarkNodeUp(const PmidName& pmid_node, const DataName& name) {
  VLOG(nfs::Persona::kDataManager, VisualiserAction::kMarkNodeUp, pmid_node.value, name.value);
  typename DataManager::Key key(name.value, DataName::data_type::Tag::kValue);
  DoSync(DataManager::UnresolvedNodeUp(key,
             ActionDataManagerNodeUp(pmid_node), routing_.kNodeId()));
}

// ==================== Sync =======================================================================

template <typename UnresolvedAction>
void DataManagerService::DoSync(const UnresolvedAction& unresolved_action) {
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_puts_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_deletes_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_add_pmids_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_remove_pmids_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_node_downs_, unresolved_action);
  detail::IncrementAttemptsAndSendSync(dispatcher_, sync_node_ups_, unresolved_action);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
