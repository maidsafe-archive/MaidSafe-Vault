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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/data_manager/helpers.h"
#include "maidsafe/vault/data_manager/handler.h"
#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/sync.h"


namespace maidsafe {

namespace vault {

class DataManagerService {
 public:
//  typedef boost::variant<MaidNodePut, MaidNodeDelete> Messages;
  DataManagerService(const passport::Pmid& pmid,
                     routing::Routing& routing,
                     nfs_client::DataGetter& data_getter);
  template<typename T>
  void HandleMessage(const T&, const typename T::Sender& , const typename T::Receiver&);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

 private:
  template<typename Data>
  struct GetHandler {
    GetHandler(const routing::ReplyFunctor& reply_functor_in,
               size_t holder_count_in,
               const nfs::MessageId& message_id_in);
    routing::ReplyFunctor reply_functor;
    size_t holder_count;
    nfs::MessageId message_id;
    std::mutex mutex;
    crypto::SHA512Hash validation_result;
    std::vector<protobuf::DataOrProof> pmid_node_results;

   private:
    GetHandler(const GetHandler&);
    GetHandler& operator=(const GetHandler&);
    GetHandler(GetHandler&&);
    GetHandler& operator=(GetHandler&&);
  };

  DataManagerService(const DataManagerService&);
  DataManagerService& operator=(const DataManagerService&);
  DataManagerService(DataManagerService&&);
  DataManagerService& operator=(DataManagerService&&);

  template<typename T>
  void HandlePut(const T&, const typename T::Sender&, const typename T::Receiver&);

  template<typename T>
  void HandleGet(const T&, const typename T::Sender&, const typename T::Receiver&);

  void HandleDelete(const nfs::DeleteRequestFromMaidManagerToDataManager& message,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Sender& sender,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Receiver& receiver);

  void HandleStateChange(const nfs::StateChangeFromPmidManagerToDataManager& message,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Sender& sender,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Receiver& receiver);

  template<typename Data>
  void OnHandleGet(std::shared_ptr<GetHandler<Data>> get_handler,
                   const std::string& serialised_reply);
  template<typename Data>
  void IntegrityCheck(std::shared_ptr<GetHandler<Data>> get_handler);

  template<typename T>
  void ValidateSender(const T& message, const typename T::Sender& sender) const;
//  void ValidatePutSender(const nfs::Message& message) const;
//  void ValidateGetSender(const nfs::Message& message) const;
//  void ValidateDeleteSender(const nfs::Message& message) const;
//  void ValidatePostSender(const nfs::Message& message) const;
//  void ValidatePutResultSender(const nfs::Message& message) const;

/* Commented by Mahmoud on 3 Sep. Code need refactoring
  void HandleNodeDown(const nfs::Message& message);
  void HandleNodeUp(const nfs::Message& message);
  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result);
  template<typename Data>
  void HandleGetReply(std::string serialised_reply);
  template<typename Data>
  void OnGenericErrorHandler(nfs::Message message);
  bool ThisVaultInGroupForData(const nfs::Message& message) const;
  template<typename Data, nfs::MessageAction Action>
  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message,
                                       const DataManagerValue& metadata_value);
*/

  // =============== Sync and Record transfer =====================================================
//  void Sync();
/* Commented by Mahmoud on 3 Sep. Code need refactoring
  void HandleSync(const nfs::Message& message); */
  void TransferRecord(const DataNameVariant& record_name, const NodeId& new_node);
/* Commented by Mahmoud on 3 Sep. Code need refactoring
  void HandleRecordTransfer(const nfs::Message& message);
*/
  routing::Routing& routing_;
  nfs_client::DataGetter& public_key_getter_;
  std::mutex accumulator_mutex_;
  Accumulator<DataNameVariant> accumulator_;
  GroupDb<DataManager> group_db_;
  MetadataHandler metadata_handler_;
  Sync<DataManager::UnresolvedPut> sync_puts_;
  Sync<DataManager::UnresolvedDelete> sync_deletes_;
  Sync<DataManager::UnresolvedAddPmid> sync_add_pmids_;
  Sync<DataManager::UnresolvedRemovePmid> sync_remove_pmids_;
  Sync<DataManager::UnresolvedNodeDown> sync_node_downs_;
  Sync<DataManager::UnresolvedNodeUp> sync_node_ups_;
  static const int kPutRequestsRequired_;
  static const int kStateChangesRequired_;
  static const int kDeleteRequestsRequired_;
};

// =========================== Handle Message Specialisations ======================================

template<typename T>
void HandleMessage(const T& /*message*/,
                   const typename T::Sender& /*sender*/,
                   const typename T::Receiver& /*receiver*/) {
  T::should_not_reach_here;
}

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromMaidNodeToDataManager& message,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromPmidNodeToDataManager& message,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromDataGetterToDataManager& message,
   const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
   const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::PutRequestFromMaidManagerToDataManager& message,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Sender& sender,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::DeleteRequestFromMaidManagerToDataManager& message,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Sender& sender,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::PutResponseFromPmidManagerToDataManager& message,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Sender& sender,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::StateChangeFromPmidManagerToDataManager& message,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Sender& sender,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetResponseFromPmidNodeToDataManager& message,
   const typename nfs::GetResponseFromPmidNodeToDataManager::Sender& sender,
   const typename nfs::GetResponseFromPmidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleMessage(
   const nfs::SynchroniseFromDataManagerToDataManager& message,
   const typename nfs::SynchroniseFromDataManagerToDataManager::Sender& sender,
   const typename nfs::SynchroniseFromDataManagerToDataManager::Receiver& receiver);


// ============================== Handle Get Specialisations ======================================

template<typename T>
void DataManagerService::HandleGet(const T&,
                                   const typename T::Sender&,
                                   const typename T::Receiver&) {
  T::should_not_reach_here;
}

template<>
void DataManagerService::HandleGet(
  const nfs::GetRequestFromMaidNodeToDataManager& message,
  const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
  const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleGet(
   const nfs::GetRequestFromPmidNodeToDataManager& message,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandleGet(
   const nfs::GetRequestFromDataGetterToDataManager& message,
   const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender,
   const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver);

// ============================== Handle Put Specialisations ======================================

template<typename T>
void DataManagerService::HandlePut(const T&,
                                   const typename T::Sender&,
                                   const typename T::Receiver&) {
  T::should_not_reach_here;
}

template<>
void DataManagerService::HandlePut(
    const nfs::PutRequestFromMaidManagerToDataManager& message,
    const typename nfs::PutRequestFromMaidManagerToDataManager::Sender& sender,
    const typename nfs::PutRequestFromMaidManagerToDataManager::Receiver& receiver);

template<>
void DataManagerService::HandlePut(
   const nfs::PutResponseFromPmidManagerToDataManager& message,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Sender& sender,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Receiver& receiver);


}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/data_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
