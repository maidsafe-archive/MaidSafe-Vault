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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_HANDLER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_HANDLER_H_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/types.h"


//namespace maidsafe {

//namespace vault {

//struct PmidManagerMetadata;

//namespace test { class MaidManagerHandlerTest; }


//class MaidAccountHandler {
// public:
//  MaidAccountHandler(Db& db, const NodeId& this_node_id);

//  // Account operations
//  // Returns true when final account transfer has been completed.
//  bool ApplyAccountTransfer(const MaidName& account_name,
//                            const NodeId& source_id,
//                            const MaidAccount::serialised_type& serialised_maid_account_details);
//  // client request or going out of range
//  void DeleteAccount(const MaidName& account_name);

//  void RegisterPmid(const MaidName& account_name, const nfs_vault::PmidRegistration& pmid_registration);
//  void UnregisterPmid(const MaidName& account_name, const PmidName& pmid_name);
//  std::vector<PmidName> GetPmidNames(const MaidName& account_name) const;
//  void UpdatePmidTotals(const MaidName& account_name, const PmidManagerMetadata& pmid_totals);
//  void AddLocalUnresolvedAction(const MaidName& account_name,
//                               const MaidManagerUnresolvedAction& unresolved_action);

//  // Sync operations
//  std::vector<MaidName> GetAccountNames() const;
//  MaidAccount::serialised_type GetSerialisedAccount(const MaidName& account_name) const;
//  NonEmptyString GetSyncData(const MaidName& account_name);
//  void ApplySyncData(const MaidName& account_name,
//                     const NonEmptyString& serialised_unresolved_actions);
//  void ReplaceNodeInSyncList(const MaidName& account_name,
//                             const NodeId& old_node,
//                             const NodeId& new_node);
//  void IncrementSyncAttempts(const MaidName& account_name);

//  // Data operations
//  MaidAccount::Status AllowPut(const MaidName& account_name, int32_t cost) const;

//  // Only Maid and Anmaid can create account; for all others this is a no-op.
//  typedef std::true_type AllowedAccountCreationType;
//  typedef std::false_type DisallowedAccountCreationType;
//  template<typename Data>
//  void CreateAccount(const MaidName& account_name, AllowedAccountCreationType);
//  template<typename Data>
//  void CreateAccount(const MaidName& /*account_name*/, DisallowedAccountCreationType) {}

//  template<typename Data>
//  void DeleteData(const MaidName& account_name, const typename Data::Name& data_name);

//  friend class test::MaidManagerHandlerTest;

// private:
//  MaidAccountHandler(const MaidAccountHandler&);
//  MaidAccountHandler& operator=(const MaidAccountHandler&);
//  MaidAccountHandler(MaidAccountHandler&&);
//  MaidAccountHandler& operator=(MaidAccountHandler&&);

//  Db& db_;
//  const NodeId kThisNodeId_;
//  mutable std::mutex mutex_;
//  std::map<typename MaidAccount::Name, std::unique_ptr<MaidAccount>> maid_accounts_;
//};

//}  // namespace vault

//}  // namespace maidsafe

//#include "maidsafe/vault/maid_manager/handler-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_HANDLER_H_
