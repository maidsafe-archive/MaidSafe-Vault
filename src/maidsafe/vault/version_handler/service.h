/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_SERVICE_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_SERVICE_H_

#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/storage_merge/storage_merge.h"
#include "maidsafe/vault/version_handler/version_handler.h"
//#include "maidsafe/vault/manager_db.h"

namespace maidsafe {

namespace vault {

class VersionHandlerService {
 public:
  typedef nfs::VersionHandlerServiceMessages PublicMessages;
  typedef void VaultMessages;
  typedef nfs::VersionHandlerServiceMessages Messages;
  typedef void HandleMessageReturnType;
  typedef Identity VersionHandlerAccountName;

  VersionHandlerService(const passport::Pmid& pmid, routing::Routing& routing);
  template<typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}

 private:
  VersionHandlerService(const VersionHandlerService&);
  VersionHandlerService& operator=(const VersionHandlerService&);
  VersionHandlerService(VersionHandlerService&&);
  VersionHandlerService& operator=(VersionHandlerService&&);

  template <typename MessageType>
  bool ValidateSender(const MessageType& message, const typename MessageType::Sender& sender) const;

  template <typename RequestorType>
  void HandleGet(const VersionHandler::Key& key, const RequestorType& requestor_type);

  //  std::vector<StructuredDataVersions::VersionName> GetVersionsFromMessage(
  //      const nfs::Message& message) const;
  //  NonEmptyString GetSerialisedRecord(const VersionHandler::DbKey& db_key);
  //  //// =============== Get data
  // ====================================================================
  //  void HandleGetBranch(const nfs::Message& message, routing::ReplyFunctor reply_functor);

  //  //// =============== Sync
  // ========================================================================
  //  template<typename Data>
  //  void Synchronise(const nfs::Message& message);
  //  void HandleSynchronise(const nfs::Message& message);

  //  //// =============== Churn
  // =======================================================================
  //  void HandleAccountTransfer(const nfs::Message& message);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  std::mutex sync_mutex_;
  Accumulator<Messages> accumulator_;
  Db<VersionHandler::Key, VersionHandler::Value> db_;
  const NodeId kThisNodeId_;
};

template<typename MessageType>
void VersionHandlerService::HandleMessage(const MessageType& /*message*/,
                                          const typename MessageType::Sender& /*sender*/,
                                          const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_generic_handler_available__Specialisation_is_required;
}

template <typename MessageType>
bool VersionHandlerService::ValidateSender(const MessageType& /*message*/,
                                           const typename MessageType::Sender& /*sender*/) const {
  return false;
}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& receiver);

template <typename RequestorType>
void VersionHandlerService::HandleGet(const VersionHandler::Key& /*key*/,
                                      const RequestorType& /*requestor_type*/) {
// Implementing the function requires handling structured version transfer
//  auto value(db_.Get(key));
//  try {
//    dispatcher_.SendGetVersionResponse(value, requestor_type);
//  }
//  catch (const maidsafe_error& error) {
//    dispatcher_.SendGetVersionResponse(value, requestor_type, error);
//  }
}

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_SERVICE_H_
