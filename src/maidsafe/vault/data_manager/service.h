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
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/nfs/message_types.h"

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

class DataManagerService {
 public:
  typedef nfs::DataManagerServiceMessages PublicMessages;
  typedef DataManagerServiceMessages VaultMessages;
  typedef void HandleMessageReturnType;

  DataManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter);
  template <typename T>
  void HandleMessage(const T& message,
                     const typename T::Sender& sender,
                     const typename T::Receiver& receiver);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

  template <typename ServiceHandlerType, typename RequestorIdType>
  friend class detail::GetRequestVisitor;
  template<typename ServiceHandlerType>
  friend class detail::DataManagerSendDeleteVisitor;

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

  template <typename Data>
  void HandlePutResponse(const typename Data::name& data_name, const PmidName& pmid_node,
                         int32_t size, nfs::MessageId message_id);

  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data_name, const PmidName& attempted_pmid_node,
                        nfs::MessageId message_id, const maidsafe_error& error);

  template <typename DataName>
  bool SendPutRetryRequired(const DataName& data_name);

  // =========================== Get section (includes integrity checks) ===========================
  typedef GetResponseFromPmidNodeToDataManager::Contents GetResponseContents;

  template <typename Data, typename RequestorIdType>
  void HandleGet(const typename Data::Name& data_name, const RequestorIdType& requestor,
                 nfs::MessageId message_id);

  void HandleGetResponse(const PmidName& pmid_name, nfs::MessageId message_id,
                         const GetResponseContents& contents);

  // Removes a pmid_name from the set and returns it.
  template <typename DataName>
  PmidName ChoosePmidNodeToGetFrom(std::set<PmidName>& online_pmids,
                                   const DataName& data_name) const;

  template <typename Data, typename RequestorIdType>
  void DoHandleGetResponse(
      const PmidName& pmid_node, const GetResponseContents& contents,
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data, typename RequestorIdType>
  bool SendGetResponse(
      const GetResponseContents& contents,
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  template <typename Data, typename RequestorIdType>
  void AssessIntegrityCheckResults(
      std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op);

  // =========================== Delete section ====================================================
  template <typename Data>
  void HandleDelete(const typename Data::Name& data_name, nfs::MessageId message_id);

  void SendDeleteRequests(const DataManager::Key& key, const std::set<PmidName>& pmids,
                          nfs::MessageId message_id);

  template <typename Data>
  void SendDeleteRequest(const PmidName pmid_node, const typename Data::Name& name,
                         nfs::MessageId message_id);

  // =========================== Sync / AccountTransfer section ====================================
  void DoSync();

  // =========================== General functions =================================================










  void HandleDataIntegrityResponse(const GetResponseContents& response, nfs::MessageId message_id);

  template <typename Data>
  NonEmptyString GetContentFromCache(const typename Data::Name& name);

  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const {
    // Don't need to check sender or receiver type - only need to check for group sources that
    // sender ID is appropriate (i.e. == MaidName, or == DataName).
    return false;
  }

  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   nfs::DataManagerServiceMessages::types>::type IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   DataManagerServiceMessages::types>::type FinalType;
 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  routing::Routing& routing_;
  AsioService asio_service_;
  nfs_client::DataGetter& data_getter_;
  mutable std::mutex accumulator_mutex_, matrix_change_mutex_;
  Accumulator<Messages> accumulator_;
  routing::MatrixChange matrix_change_;
  DataManagerDispatcher dispatcher_;
  routing::Timer<std::pair<PmidName, GetResponseContents>> get_timer_;
  Db<DataManager::Key, DataManager::Value> db_;
  Sync<DataManager::UnresolvedPut> sync_puts_;
  Sync<DataManager::UnresolvedDelete> sync_deletes_;
  Sync<DataManager::UnresolvedAddPmid> sync_add_pmids_;
  Sync<DataManager::UnresolvedRemovePmid> sync_remove_pmids_;
  Sync<DataManager::UnresolvedNodeDown> sync_node_downs_;
  Sync<DataManager::UnresolvedNodeUp> sync_node_ups_;
};

// =========================== Handle Message Specialisations ======================================
template <typename T>
void DataManagerService::HandleMessage(const T&, const typename T::Sender&,
                                       const typename T::Receiver&) {}

template <>
void DataManagerService::HandleMessage(
    const PutRequestFromMaidManagerToDataManager& message,
    const typename PutRequestFromMaidManagerToDataManager::Sender&,
    const typename PutRequestFromMaidManagerToDataManager::Receiver&);

template <>
void DataManagerService::HandleMessage(
    const PutFailureFromPmidManagerToDataManager& message,
    const typename PutFailureFromPmidManagerToDataManager::Sender& sender,
    const typename PutFailureFromPmidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
    const nfs::GetRequestFromDataGetterToDataManager& message,
    const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
    const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
    const GetResponseFromPmidNodeToDataManager& message,
    const typename GetResponseFromPmidNodeToDataManager::Sender& sender,
    const typename GetResponseFromPmidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
    const DeleteRequestFromMaidManagerToDataManager& message,
    const typename DeleteRequestFromMaidManagerToDataManager::Sender& sender,
    const typename DeleteRequestFromMaidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const SynchroniseFromDataManagerToDataManager& message,
   const typename SynchroniseFromDataManagerToDataManager::Sender& sender,
   const typename SynchroniseFromDataManagerToDataManager::Receiver& receiver);

// ==================== Put implementation =========================================================
template <typename Data>
void DataManagerService::HandlePut(const Data& data, const MaidName& maid_name,
                                   const PmidName& pmid_name_in, nfs::MessageId message_id) {
  int32_t cost(data.data().string().size());
  if (!EntryExist<Data>(data.name())) {
    cost *= routing::Parameters::node_group_size;
    PmidName pmid_name;
    if (routing_.ClosestToId(data.name()))
      pmid_name = pmid_name_in;
    else
      pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));
    dispatcher_.SendPutRequest(pmid_name, data, message_id);
  } else if (is_unique_on_network<Data>::value) {
    dispatcher_.SendPutFailure<Data>(maid_name, data.name(),
                                     maidsafe_error(VaultErrors::unique_data_clash), message_id);
    return;
  } else {
    typename DataManager::Key key(data.name().raw_name, Data::Name::data_type);
    sync_puts_.AddLocalAction(DataManager::UnresolvedPut(key, ActionDataManagerPut(),
                                                         routing_.kNodeId()));
    DoSync();
  }
  dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id); // NOT IN FAILURE
}

template <typename Data>
bool DataManagerService::EntryExist(const typename Data::Name& name) {
  try {
    auto value(db_.Get(DataManager::Key(name.value, Data::Tag::kValue)));
    return value;
  }
  catch (const maidsafe_error& /*error*/) {}
  return false;
}

template <typename Data>
void DataManagerService::HandlePutResponse(const typename Data::name& data_name,
                                           const PmidName& pmid_node, int32_t size,
                                           nfs::MessageId /*message_id*/) {
  typename DataManager::Key key(data_name.raw_name, data_name.type);
  sync_add_pmids_.AddLocalAction(DataManager::UnresolvedAddPmid(
      key, ActionDataManagerAddPmid(pmid_node, size), routing_.kNodeId()));
  DoSync();
}

template <typename Data>
void DataManagerService::HandlePutFailure(const typename Data::Name& data_name,
                                          const PmidName& attempted_pmid_node,
                                          nfs::MessageId message_id,
                                          const maidsafe_error& /*error*/) {
  // TODO(Team): Following should be done only if error is fixable by repeat

  // Get all pmid nodes for this data.
  if (SendPutRetryRequired(data_name)) {
    std::set<PmidName> pmids_to_avoid;
    auto value(db_.Get(data_name));
    if (value)
      pmids_to_avoid = value->AllPmids();

    pmids_to_avoid.insert(attempted_pmid_node);
    auto pmid_name(PmidName(Identity(routing_.RandomConnectedNode().string())));
    while (pmids_to_avoid.find(pmid_name) != std::end(pmids_to_avoid))
      pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));

    try {
      NonEmptyString content(GetContentFromCache<Data>(data_name));
      dispatcher_.SendPutRequest(pmid_name, Data(data_name, content), message_id);
    }
    catch (std::exception& /*ex*/) {
      // handle failure to retrieve content from cache, a Get->Then->call
      // dispatcher_.SendPutRequest(pmid_name, Data(data_name, content), message_id); )
    }
  }
  typename DataManager::Key key(data_name.raw_name, data_name.type);
  sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
      key, ActionDataManagerRemovePmid(attempted_pmid_node), routing_.kNodeId()));
  DoSync();
}

template <typename DataName>
bool DataManagerService::SendPutRetryRequired(const DataName& data_name) {
  try {
    // mutex is required
    auto value(db_.Get(DataManager::Key(data_name.value, DataName::data_type::Tag::kValue)));
    return value && value->AllPmids().size() < routing::Parameters::node_group_size;
  }
  catch (const maidsafe_error& /*error*/) {}
  return false;
}

// ==================== Get / IntegrityCheck implementation ========================================
template <typename Data, typename RequestorIdType>
void DataManagerService::HandleGet(const typename Data::Name& data_name,
                                   const RequestorIdType& requestor,
                                   nfs::MessageId message_id) {
  // Get all pmid nodes that are online.
  auto value(db_.Get(data_name));
  if (!value) {
    // TODO(Fraser#5#): 2013-10-03 - Request for non-existent data should possibly generate an alert
    LOG(kWarning) << HexEncode(data_name) << " doesn't exist.";
    return;
  }
  auto online_pmids(value->online_pmids());
  int expected_response_count(static_cast<int>(online_pmids.size()));

  // Choose the one we're going to ask for actual data, and set up the others for integrity checks.
  auto pmid_node_to_get_from(ChoosePmidNodeToGetFrom(online_pmids, data_name));
  std::map<PmidName, IntegrityCheckData> integrity_checks;
  auto hint_itr(std::end(integrity_checks));
  std::for_each(std::begin(online_pmids), std::end(online_pmids),
                [&](PmidName&& name) {
                  hint_itr = integrity_checks.insert(hint_itr, std::make_pair(std::move(name),
                      IntegrityCheckData(IntegrityCheckData::GetRandomInput())));
                });

  // Create helper struct which holds the collection of responses, and add the task to the timer.
  auto get_response_op(
      std::make_shared<detail::GetResponseOp<typename Data::Name, RequestorIdType>>(
          pmid_node_to_get_from, integrity_checks, data_name, requestor));
  auto functor([=](const std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    this->DoHandleGetResponse(pmid_node_and_contents.first, pmid_node_and_contents.second,
                              get_response_op);
  });
  get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, expected_response_count,
                     message_id.data);

  // Send requests
  dispatcher_.SendGetRequest(pmid_node_to_get_from, data_name, message_id);

  for (const auto& integrity_check : integrity_checks) {
    dispatcher_.SendIntegrityCheck(data_name, NonEmptyString(integrity_check.second.random_input()),
                                   integrity_check.first, message_id);
  }
}

template <typename DataName>
PmidName DataManagerService::ChoosePmidNodeToGetFrom(std::set<PmidName>& online_pmids,
                                                     const DataName& data_name) const {
  // Convert the set of PmidNames to a set of NodeIds
  std::set<NodeId> online_node_ids;
  auto hint_itr(std::end(online_node_ids));
  std::for_each(std::begin(online_pmids), std::end(online_pmids),
                [&](const PmidName& name) {
                  hint_itr = online_node_ids.insert(hint_itr, NodeId(name->string()));
                });

  PmidName chosen;
  {
    std::lock_guard<std::mutex> lock(matrix_change_mutex_);
    chosen = PmidName(Identity(
        matrix_change_.ChoosePmidNode(online_node_ids, NodeId(data_name->string())).string()));
  }

  online_pmids.erase(chosen);
  return chosen;
}

template <typename Data, typename RequestorIdType>
void DataManagerService::DoHandleGetResponse(
    const PmidName& pmid_node, const GetResponseContents& contents,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  // Note: if 'contents' is default-constructed, it's probably a result of this function being
  // invoked by the timer after timeout.
  int called_count(0), expected_count(0);
  {
    std::lock_guard<std::mutex> lock(get_response_op->mutex);
    called_count = ++get_response_op->called_count;
    expected_count = static_cast<int>(get_response_op->integrity_checks.size()) + 1;
    assert(called_count <= expected_count);
    if (pmid_node == get_response_op->pmid_node_to_get_from) {
      if (SendGetResponse(contents, get_response_op)) {
        get_response_op->serialised_contents = typename Data::serialised_type(contents.content);
      }
    } else if (contents.check_result) {
      auto itr(get_response_op->integrity_checks.find(pmid_node));
      if (itr != std::end(get_response_op->integrity_checks))
        itr->second.SetResult(*contents.check_result);
    }
  }
  if (called_count == expected_count)
    AssessIntegrityCheckResults(get_response_op);
}

template <typename Data, typename RequestorIdType>
bool DataManagerService::SendGetResponse(
    const GetResponseContents& contents,
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  maidsafe_error error(MakeError(CommonErrors::unknown));
  try {
    if (!contents.content)
      ThrowError(CommonErrors::unknown);
    Data data(get_response_op->data_name, typename Data::serialised_type(contents.content));
    dispatcher_.SendGetResponseSuccess(get_response_op->requestor_id, data,
                                       get_response_op->message_id);
    // Put to the CacheHandler in this vault.
    dispatcher_.SendPutToCache(data);
    return true;
  } catch(const maidsafe_error& e) {
    error = e;
    LOG(kError) << e.what();
  } catch(const std::exception& e) {
    LOG(kError) << e.what();
  } catch(...) {
    LOG(kError) << "Unexpected exception type.";
  }
  dispatcher_.SendGetResponseFailure(get_response_op->requestor_id, get_response_op->data_name,
                                     error, get_response_op->message_id);
  return false;
}

template <typename Data, typename RequestorIdType>
void DataManagerService::AssessIntegrityCheckResults(
    std::shared_ptr<detail::GetResponseOp<typename Data::Name, RequestorIdType>> get_response_op) {
  // If we failed to get the serialised_contents, mark 'pmid_node_to_get_from' as down, sync this,
  // try to Get from peer DataManagers' caches.
  if (!get_response_op->serialised_contents->IsInitialised()) {
  }
}

//template <typename Data>
//void DataManagerService::SendIntegrityCheck(const typename Data::name& data_name,
//                                            const PmidName& pmid_node,
//                                            nfs::MessageId message_id) {
//  try {
//    NonEmptyString data(GetContentFromCache(data_name));
//    std::string random_string(RandomString(detail::Parameters::integrity_check_string_size));
//    NonEmptyString signature(
//        crypto::Hash<crypto::SHA512>(NonEmptyString(data.string() + random_string)));
//    get_timer_.AddTask(
//        std::chrono::seconds(10),
//        [signature, pmid_node, message_id, data_name, this](
//            DataManagerService::IntegrityCheckResponse response) {
//          if (response == DataManagerService::IntegrityCheckResponse()) {
//            // Timer expired.
//            // If PN has informed PMs about any failure the request from PMs should have arrived.
//            // If PN is still in DM's PNs, the PN is too slow or not honest. Therefore, should be
//            // removed from DM's PNs and deranked. Moreover the PMs must be informed.
//            if (HasPmidNode<Data>(data_name, pmid_node)) {
//              dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
//              sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
//                  typename DataManager::Key(data_name.raw_name, data_name.type),
//                  ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
//              DoSync();
//              return;
//            }
//          }
//          if (response.return_code.value.code() != CommonErrors::success) {
//            // Data not available on pmid , sync remove pmid_node, inform PMs
//            dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
//            sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
//                typename DataManager::Key(data_name.raw_name, data_name.type),
//                ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
//            DoSync();
//            return;
//          }
//          if (response.return_code.value.code() == CommonErrors::success) {
//            if (response.signature != signature) {
//              // Lieing pmid_node, sync remove pmid_node, inform PMs and drank
//              dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
//              sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
//                  typename DataManager::Key(data_name.raw_name, data_name.type),
//                  ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
//              DoSync();
//              return;
//            }
//          }
//        },
//        1, message_id.data);
//    dispatcher_.SendIntegrityCheck(data_name, random_string, pmid_node, nfs::MessageId(message_id));
//  }
//  catch (const std::exception& /*ex*/) {
//    // handle failure to retrieve from cache
//  }
//}

// ==================== Delete implementation ======================================================
template <typename Data>
void DataManagerService::HandleDelete(const typename Data::Name& data_name,
                                      nfs::MessageId /*message_id*/) {
  typename DataManager::Key key(data_name.value, Data::Name::data_type::Tag::kValue);
  sync_deletes_.AddLocalAction(DataManager::UnresolvedDelete(key, ActionDataManagerDelete(),
                                                             routing_.kNodeId()));
  DoSync();
}

template <typename Data>
void DataManagerService::SendDeleteRequest(
    const PmidName pmid_node, const typename Data::Name& name, nfs::MessageId message_id) {
  dispatcher_.SendDeleteRequest<Data>(pmid_node, name, message_id);
}

// ==================== General implementation =====================================================

namespace detail {

template <typename DataManagerSyncType>
void IncrementAttemptsAndSendSync(DataManagerDispatcher& dispatcher,
                                  DataManagerSyncType& sync_type) {
  auto unresolved_actions(sync_type.GetUnresolvedActions());
  if (!unresolved_actions.empty()) {
    sync_type.IncrementSyncAttempts();
    for (const auto& unresolved_action : unresolved_actions)
      dispatcher.SendSync(unresolved_action->key.name, unresolved_action->Serialise());
  }
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
