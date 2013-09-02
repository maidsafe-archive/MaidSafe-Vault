/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

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


namespace maidsafe {

namespace vault {

class DataManagerService {
 public:
  typedef boost::variant<MaidNodePut, MaidNodeDelete> Messages;
  DataManagerService(const passport::Pmid& pmid,
                         routing::Routing& routing);
  template<typename T>
  void HandleMessage(const T& message,
                     const typename T::Receiver& receiver,
                     const typename T::Sender& sender);
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleMessage(const nfs::Message& /*message*/, const routing::ReplyFunctor& /*reply_functor*/) {}
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

  template<typename Data>
  void HandlePut(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void Put(const Data& data, const PmidName& target_pmid_node);
  template<typename Data>
  void HandlePutResult(const nfs::Message& message);
  template<typename Data>
  void HandleGet(nfs::Message message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void OnHandleGet(std::shared_ptr<GetHandler<Data>> get_handler,
                   const std::string& serialised_reply);
  template<typename Data>
  void IntegrityCheck(std::shared_ptr<GetHandler<Data>> get_handler);
  template<typename Data>
  void HandleDelete(const nfs::Message& message);

  template<typename Data>
  void HandleStateChange(const nfs::Message& message);

  void ValidatePutSender(const nfs::Message& message) const;
  void ValidateGetSender(const nfs::Message& message) const;
  void ValidateDeleteSender(const nfs::Message& message) const;
  void ValidatePostSender(const nfs::Message& message) const;
  void ValidatePutResultSender(const nfs::Message& message) const;

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

  // =============== Sync and Record transfer =====================================================
  void Sync();
  void HandleSync(const nfs::Message& message);
  void TransferRecord(const DataNameVariant& record_name, const NodeId& new_node);
  void HandleRecordTransfer(const nfs::Message& message);
  routing::Routing& routing_;
  nfs_client::DataGetter& public_key_getter_;
  std::mutex accumulator_mutex_;
  Accumulator<DataNameVariant> accumulator_;
  MetadataHandler metadata_handler_;
  DataManagerNfs nfs_;
  static const int kPutRequestsRequired_;
  static const int kStateChangesRequired_;
  static const int kDeleteRequestsRequired_;
};

template<typename T>
void HandleMessage(const T& /*message*/,
                   const typename T::Receiver& /*receiver*/,
                   const typename T::Sender& /*sender*/) {
  T::should_not_reach_here;
}

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromMaidNodeToDataManager& message,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver,
   const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromPmidNodeToDataManager& message,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver,
   const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetRequestFromDataGetterToDataManager& message,
   const typename nfs::GetRequestFromDataGetterToDataManager::Receiver& receiver,
   const typename nfs::GetRequestFromDataGetterToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::PutRequestFromMaidManagerToDataManager& message,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Receiver& receiver,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::DeleteRequestFromMaidManagerToDataManager& message,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Receiver& receiver,
   const typename nfs::DeleteRequestFromMaidManagerToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::PutResponseFromPmidManagerToDataManager& message,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Receiver& receiver,
   const typename nfs::PutResponseFromPmidManagerToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::StateChangeFromPmidManagerToDataManager& message,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Receiver& receiver,
   const typename nfs::StateChangeFromPmidManagerToDataManager::Sender& sender);

template<>
void DataManagerService::HandleMessage(
   const nfs::GetResponseFromPmidNodeToDataManager& message,
   const typename nfs::GetResponseFromPmidNodeToDataManager::Receiver& receiver,
   const typename nfs::GetResponseFromPmidNodeToDataManager::Sender& sender);

}  // namespace vault

}  // namespace maidsafe

//#include "maidsafe/vault/data_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_SERVICE_H_
