/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_SERVICE_H_

#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/storage_merge/storage_merge.h"
#include "maidsafe/vault/version_manager/version_manager.h"
//#include "maidsafe/vault/manager_db.h"


namespace maidsafe {

namespace vault {

class VersionManagerService {
 public:
  typedef nfs::VersionManagerServiceMessages PublicMessages;
  typedef nfs::VersionManagerServiceMessages VaultMessages; // FIXME (Check with Fraser)

  typedef Identity VersionManagerAccountName;
  VersionManagerService(const passport::Pmid& pmid, routing::Routing& routing);
  template<typename Data>
//  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);

 private:
  VersionManagerService(const VersionManagerService&);
  VersionManagerService& operator=(const VersionManagerService&);
  VersionManagerService(VersionManagerService&&);
  VersionManagerService& operator=(VersionManagerService&&);

//  void ValidateClientSender(const nfs::Message& message) const;
//  void ValidateSyncSender(const nfs::Message& message) const;
//  std::vector<StructuredDataVersions::VersionName> GetVersionsFromMessage(
//      const nfs::Message& message) const;
//  NonEmptyString GetSerialisedRecord(const VersionManager::DbKey& db_key);
//  //// =============== Get data ====================================================================
//  void HandleGet(const nfs::Message& message, routing::ReplyFunctor reply_functor);
//  void HandleGetBranch(const nfs::Message& message, routing::ReplyFunctor reply_functor);

//  //// =============== Sync ========================================================================
//  template<typename Data>
//  void Synchronise(const nfs::Message& message);
//  void HandleSynchronise(const nfs::Message& message);

//  //// =============== Churn =======================================================================
//  void HandleAccountTransfer(const nfs::Message& message);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  std::mutex sync_mutex_;
  Accumulator<VersionManagerAccountName> accumulator_;
  Db<VersionManagerKey, StructuredDataVersions> version_manager_db_;
  const NodeId kThisNodeId_;
//  Sync<VersionManagerMergePolicy> sync_;
//  VersionManagerNfs nfs_;
//  StorageMerge<VersionManagerKey,
//               StructuredDataVersions,
//               ManagerDb<VersionManagerKey,
//                         StructuredDataVersions>>database_merge_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/version_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_SERVICE_H_
