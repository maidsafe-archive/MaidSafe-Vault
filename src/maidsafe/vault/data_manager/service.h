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

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/operations_visitor.h"
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

  DataManagerService(const passport::Pmid& pmid, routing::Routing& routing,
                     nfs_client::DataGetter& data_getter);
  template <typename T>
  void HandleMessage(const T&, const typename T::Sender&, const typename T::Receiver&);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}

  template <typename ServiceHandlerType, typename Requestor>
  friend class detail::GetRequestVisitor;
  template<typename ServiceHandlerType>
  friend class detail::DataManagerSendDeleteVisitor;

 private:
  typedef GetResponseFromPmidNodeToDataManager::Contents GetResponseContents;

  template <typename Data>
  void HandlePut(const Data& data, const MaidName& maid_name, const PmidName& pmid_name_in,
                 const nfs::MessageId& message_id);

  template <typename Data>
  void HandlePutResponse(const typename Data::name& data_name, const PmidName& pmid_node,
                         int32_t size, const nfs::MessageId& message_id);
  // Failure case
  template <typename Data>
  void HandlePutFailure(const typename Data::Name& data_name, const PmidName& attempted_pmid_node,
                        const nfs::MessageId& message_id, const maidsafe_error& error);

  template <typename Data, typename Requestor>
  void HandleGet(const typename Data::Name& data_name, const Requestor& requestor,
                 const nfs::MessageId& message_id);

  template <typename Data>
  void HandleDelete(const typename Data::Name& data_name, const nfs::MessageId& message_id);

  void DoSync();

  template <typename Data>
  bool EntryExist(const typename Data::Name& name);

  template <typename Data>
  bool SendPutRetryRequired(const typename Data::Name& name);

  template <typename Data>
  void SendIntegrityCheck(const typename Data::name& data_name, const PmidName& pmid_node,
                          const nfs::MessageId& message_id);

  void HandleDataIntegrityResponse(const GetResponseContents& response,
                                   const nfs::MessageId& message_id);
  template <typename Data>
  bool HasPmidNode(const typename Data::Name& data_name, const PmidName& pmid_node);

  void SendDeleteRequests(const DataManager::Key& key, const std::set<PmidName>& pmids,
                          const nfs::MessageId& message_id);

  template <typename Data>
  void SendDeleteRequest(const PmidName pmid_node, const typename Data::Name& name,
                         const nfs::MessageId& message_id);

  template <typename Data>
  std::vector<PmidName> StoringPmidNodes(const typename Data::Name& name);

  template <typename Data>
  NonEmptyString GetContentFromCache(const typename Data::Name& name);

  DataManagerService(const DataManagerService&);
  DataManagerService& operator=(const DataManagerService&);
  DataManagerService(DataManagerService&&);
  DataManagerService& operator=(DataManagerService&&);

  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const {
    return false;
  }

  // =============== Sync and Record transfer =====================================================

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
  std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  DataManagerDispatcher dispatcher_;
  routing::Timer<GetResponseContents> get_timer_;
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
    const GetRequestFromPmidNodeToDataManager& message,
    const typename GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename GetRequestFromPmidNodeToDataManager::Receiver& receiver);

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

// ==================== Implementation =============================================================
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

// ================================== Put implementation ==========================================

template <typename Data>
void DataManagerService::HandlePut(const Data& data, const MaidName& maid_name,
                                   const PmidName& pmid_name_in, const nfs::MessageId& message_id) {
  int32_t cost(data.data().string().size());
  if (!EntryExist<Data>(data.name())) {
    cost *= routing::Parameters::node_group_size;
    PmidName pmid_name;
    if (routing_.ClosestToId(data.name()))
      pmid_name = pmid_name_in;
    else
      pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));
    StoreInCache(data);
    dispatcher_.SendPutRequest(pmid_name, data, message_id);
  } else {
    typename DataManager::Key key(data.name().raw_name, Data::Name::data_type);
    sync_puts_.AddLocalAction(DataManager::UnresolvedPut(key, ActionDataManagerPut(),
                                                         routing_.kNodeId(), message_id));
    DoSync();
  }
  dispatcher_.SendPutResponse<Data>(maid_name, data.name(), cost, message_id);
}

template <typename Data>
void DataManagerService::HandlePutFailure(const typename Data::Name& data_name,
                                          const PmidName& attempted_pmid_node,
                                          const nfs::MessageId& message_id,
                                          const maidsafe_error& /*error*/) {
  // TODO(Team): Following should be done only if error is fixable by repeat
  auto pmids_to_avoid(StoringPmidNodes<Data>(data_name));
  pmids_to_avoid.push_back(attempted_pmid_node);
  auto pmid_name(PmidName(Identity(routing_.RandomConnectedNode().string())));
  while (std::find(std::begin(pmids_to_avoid), std::end(pmids_to_avoid), pmid_name) !=
             std::end(pmids_to_avoid))
    pmid_name = PmidName(Identity(routing_.RandomConnectedNode().string()));
  if (SendPutRetryRequired<Data>(data_name)) {
    try {
      NonEmptyString content(GetContentFromCache<Data>(data_name));
      dispatcher_.SendPutRequest(pmid_name, Data(data_name, content), message_id);
    }
    catch (std::exception& /*ex*/) {
      // handle failure to retrieve content from cache
    }
  }
  typename DataManager::Key key(data_name.raw_name, data_name.type);
  sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
      key, ActionDataManagerRemovePmid(pmid_name), routing_.kNodeId(), message_id));
  DoSync();
}

// Success handler
template <typename Data>
void DataManagerService::HandlePutResponse(const typename Data::name& data_name,
                                           const PmidName& pmid_node, int32_t size,
                                           const nfs::MessageId& message_id) {
  typename DataManager::Key key(data_name.raw_name, data_name.type);
  sync_add_pmids_.AddLocalAction(DataManager::UnresolvedAddPmid(
      key, ActionDataManagerAddPmid(pmid_node, size), routing_.kNodeId(), message_id));
  DoSync();
}

template <typename Data, typename Requestor>
void DataManagerService::HandleGet(const typename Data::Name& /*data_name*/, const Requestor& requestor,
                                   const nfs::MessageId& /*message_id*/) {
  auto functor([requestor, this](const GetResponseContents& /*contents*/) {
    //this->HandleGetResponse(requestor, contents);
  });
  //get all pmid nodes that are up
  //choose the one we're going to ask for actual data
  //add task (requires all to reply)
  //send get request
  //send integrity checks to all others

  //get_timer_.AddTask(kDefaultTimeout, functor, , message.message_id());
//  dispatcher_.SendGetRequest(
}

template <typename Data>
void DataManagerService::SendIntegrityCheck(const typename Data::name& data_name,
                                            const PmidName& pmid_node,
                                            const nfs::MessageId& message_id) {
  try {
    NonEmptyString data(GetContentFromCache(data_name));
    std::string random_string(RandomString(detail::Parameters::integrity_check_string_size));
    NonEmptyString signature(
        crypto::Hash<crypto::SHA512>(NonEmptyString(data.string() + random_string)));
    get_timer_.AddTask(
        std::chrono::seconds(10),
        [signature, pmid_node, message_id, data_name, this](
            DataManagerService::IntegrityCheckResponse response) {
          if (response == DataManagerService::IntegrityCheckResponse()) {
            // Timer expired.
            // If PN has informed PMs about any failure the request from PMs should have arrived.
            // If PN is still in DM's PNs, the PN is too slow or not honest. Therefore, should be
            // removed from DM's PNs and deranked. Moreover the PMs must be informed.
            if (HasPmidNode<Data>(data_name, pmid_node)) {
              dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
              sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
                  typename DataManager::Key(data_name.raw_name, data_name.type),
                  ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
              DoSync();
              return;
            }
          }
          if (response.return_code.value.code() != CommonErrors::success) {
            // Data not available on pmid , sync remove pmid_node, inform PMs
            dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
            sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
                typename DataManager::Key(data_name.raw_name, data_name.type),
                ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
            DoSync();
            return;
          }
          if (response.return_code.value.code() == CommonErrors::success) {
            if (response.signature != signature) {
              // Lieing pmid_node, sync remove pmid_node, inform PMs and drank
              dispatcher_.SendDeleteRequest(pmid_node, data_name, message_id);
              sync_remove_pmids_.AddLocalAction(DataManager::UnresolvedRemovePmid(
                  typename DataManager::Key(data_name.raw_name, data_name.type),
                  ActionDataManagerRemovePmid(pmid_node), routing_.kNodeId(), message_id));
              DoSync();
              return;
            }
          }
        },
        1, message_id.data);
    dispatcher_.SendIntegrityCheck(data_name, random_string, pmid_node, nfs::MessageId(message_id));
  }
  catch (const std::exception& /*ex*/) {
    // handle failure to retrieve from cache
  }
}

template <typename Data>
void DataManagerService::HandleDelete(const typename Data::Name& data_name,
                                      const nfs::MessageId& message_id) {
  typename DataManager::Key key(data_name.value, Data::Name::data_type::Tag::kValue);
  sync_deletes_.AddLocalAction(DataManager::UnresolvedDelete(key, ActionDataManagerDelete(),
                                                             routing_.kNodeId(), message_id));
  DoSync();
}

template <typename Data>
void DataManagerService::SendDeleteRequest(
    const PmidName pmid_node, const typename Data::Name& name, const nfs::MessageId& message_id) {
  dispatcher_.SendDeleteRequest<Data>(pmid_node, name, message_id);
}

template <typename Data>
bool DataManagerService::SendPutRetryRequired(const typename Data::Name& name) {
  try {
    auto value(db_.Get(DataManager::Key(name.value, Data::Name::data_type::Tag::kValue)));
    if (!value)
      return false;
    if (value->AllPmids().size() < 3 && value->StoreFailures() > 2)
      return true;
  }
  catch (const maidsafe_error& /*error*/) {
    return false;
  }
}

template <typename Data>
bool DataManagerService::EntryExist(const typename Data::Name& name) {
  try {
    auto value(db_.Get(DataManager::Key(name.value, Data::Name::data_type::Tag::kValue)));
    if (!value)
      return false;
  }
  catch (const maidsafe_error& /*error*/) {
    return false;
  }
  return true;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
