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

#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/common/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/version_handler/dispatcher.h"

namespace maidsafe {

namespace vault {

namespace detail {

  template <typename SourcePersonaType> class VersionHandlerGetVisitor;
  template <typename SourcePersonaType> class VersionHandlerGetBranchVisitor;
  class VersionHandlerPutVisitor;
  class VersionHandlerDeleteBranchVisitor;
}

namespace test {

class VersionHandlerServiceTest;

}

class VersionHandlerService {
 public:
  typedef nfs::VersionHandlerServiceMessages PublicMessages;
  typedef VersionHandlerServiceMessages VaultMessages;
  typedef void HandleMessageReturnType;
  typedef Identity VersionHandlerAccountName;

  VersionHandlerService(const passport::Pmid& pmid, routing::Routing& routing);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

  template <typename SourcePersonaType> friend class detail::VersionHandlerGetVisitor;
  template <typename SourcePersonaType> friend class detail::VersionHandlerGetBranchVisitor;
  friend class detail::VersionHandlerPutVisitor;
  friend class detail::VersionHandlerDeleteBranchVisitor;
  friend test::VersionHandlerServiceTest;

 private:
  VersionHandlerService(const VersionHandlerService&);
  VersionHandlerService& operator=(const VersionHandlerService&);
  VersionHandlerService(VersionHandlerService&&);
  VersionHandlerService& operator=(VersionHandlerService&&);

  template <typename UnresolvedAction>
  void DoSync(const UnresolvedAction& unresolved_action);

  template <typename MessageType>
  bool ValidateSender(const MessageType& message, const typename MessageType::Sender& sender) const;

  template <typename RequestorType>
  void HandleGetVersions(const VersionHandler::Key& key, const RequestorType& requestor_type,
                         nfs::MessageId message_id);

  template <typename RequestorType>
  void HandleGetBranch(const VersionHandler::Key& key,
                       const VersionHandler::VersionName& version_name,
                       const RequestorType& requestor_type, nfs::MessageId message_id);

  void HandlePutVersion(const VersionHandler::Key& key,
                        const VersionHandler::VersionName& old_version,
                        const VersionHandler::VersionName& new_version, const NodeId& sender,
                        nfs::MessageId message_id);

  void HandleDeleteBranchUntilFork(const VersionHandler::Key& key,
                                   const VersionHandler::VersionName& branch_tip,
                                   const NodeId& sender);

  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   nfs::VersionHandlerServiceMessages::types>::type
                                       IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   VersionHandlerServiceMessages::types>::type FinalType;

 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  routing::Routing& routing_;
  VersionHandlerDispatcher dispatcher_;
  std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  Db<VersionHandler::Key, VersionHandler::Value> db_;
  const NodeId kThisNodeId_;
  Sync<VersionHandler::UnresolvedPutVersion> sync_put_versions_;
  Sync<VersionHandler::UnresolvedDeleteBranchUntilFork> sync_delete_branche_until_forks_;
};

template <typename MessageType>
void VersionHandlerService::HandleMessage(const MessageType& /*message*/,
                                          const typename MessageType::Sender& /*sender*/,
                                          const typename MessageType::Receiver& /*receiver*/) {
  MessageType::No_generic_handler_is_available__Specialisation_is_required;
}

template <typename MessageType>
bool VersionHandlerService::ValidateSender(const MessageType& /*message*/,
                                           const typename MessageType::Sender& /*sender*/) const {
  return true;  // BEFORE RELEASE
}

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetVersionsRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetVersionsRequestFromDataGetterToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromMaidNodeToVersionHandler& message,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromMaidNodeToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const nfs::GetBranchRequestFromDataGetterToVersionHandler& message,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Sender& sender,
    const typename nfs::GetBranchRequestFromDataGetterToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const PutVersionRequestFromMaidManagerToVersionHandler& message,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename PutVersionRequestFromMaidManagerToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler& message,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Sender& sender,
    const typename DeleteBranchUntilForkRequestFromMaidManagerToVersionHandler::Receiver& receiver);

template<>
void VersionHandlerService::HandleMessage(
    const SynchroniseFromVersionHandlerToVersionHandler& message,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Sender& sender,
    const typename SynchroniseFromVersionHandlerToVersionHandler::Receiver& receiver);

template <typename RequestorType>
void VersionHandlerService::HandleGetVersions(const VersionHandler::Key& key,
                                              const RequestorType& requestor_type,
                                              nfs::MessageId message_id) {
  try {
    auto value(std::move(db_.Get(key)));
    dispatcher_.SendGetVersionsResponse(key, value.Get(), requestor_type,
                                        maidsafe_error(CommonErrors::success), message_id);
  }
  catch (const maidsafe_error& error) {
    dispatcher_.SendGetVersionsResponse(key, std::vector<StructuredDataVersions::VersionName>(),
                                        requestor_type, error, message_id);
  }
}

template <typename RequestorType>
void VersionHandlerService::HandleGetBranch(
    const VersionHandler::Key& key, const VersionHandler::VersionName& version_name,
    const RequestorType& requestor_type, nfs::MessageId message_id) {
  try {
    auto value(db_.Get(key));
    dispatcher_.SendGetBranchResponse(key, value.GetBranch(version_name), requestor_type,
                                      maidsafe_error(CommonErrors::success), message_id);
  }
  catch (const maidsafe_error& error) {
    dispatcher_.SendGetBranchResponse(key, std::vector<typename VersionHandler::VersionName>(),
                                      requestor_type, error, message_id);
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_SERVICE_H_
