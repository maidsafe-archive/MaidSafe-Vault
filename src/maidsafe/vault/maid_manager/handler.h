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
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/account.h"


namespace maidsafe {

namespace vault {

struct PmidRecord;

namespace test { class MaidManagerHandlerTest; }


class MaidAccountHandler {
 public:
  MaidAccountHandler(Db& db, const NodeId& this_node_id);

  // Account operations
  // Returns true when final account transfer has been completed.
  bool ApplyAccountTransfer(const MaidName& account_name, const NodeId& source_id,
                            const MaidAccount::serialised_type& serialised_maid_account_details);
  // client request or going out of range
  void DeleteAccount(const MaidName& account_name);

  void RegisterPmid(const MaidName& account_name, const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const MaidName& account_name, const PmidName& pmid_name);
  std::vector<PmidName> GetPmidNames(const MaidName& account_name) const;
  void UpdatePmidTotals(const MaidName& account_name, const PmidRecord& pmid_totals);
  void AddLocalUnresolvedEntry(const MaidName& account_name,
                               const MaidManagerUnresolvedEntry& unresolved_entry);
  // Sync operations
  std::vector<MaidName> GetAccountNames() const;
  MaidAccount::serialised_type GetSerialisedAccount(const MaidName& account_name) const;
  NonEmptyString GetSyncData(const MaidName& account_name);
  void ApplySyncData(const MaidName& account_name,
                     const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const MaidName& account_name,
                             const NodeId& old_node,
                             const NodeId& new_node);
  void IncrementSyncAttempts(const MaidName& account_name);

  // Data operations
  MaidAccount::Status AllowPut(const MaidName& account_name, int32_t cost) const;

  // Only Maid and Anmaid can create account; for all others this is a no-op.
  typedef std::true_type AllowedAccountCreationType;
  typedef std::false_type DisallowedAccountCreationType;
  template<typename Data>
  void CreateAccount(const MaidName& account_name, AllowedAccountCreationType);
  template<typename Data>
  void CreateAccount(const MaidName& /*account_name*/, DisallowedAccountCreationType) {}

  template<typename Data>
  void DeleteData(const MaidName& account_name, const typename Data::name_type& data_name);

  friend class test::MaidManagerHandlerTest;

 private:
  MaidAccountHandler(const MaidAccountHandler&);
  MaidAccountHandler& operator=(const MaidAccountHandler&);
  MaidAccountHandler(MaidAccountHandler&&);
  MaidAccountHandler& operator=(MaidAccountHandler&&);

  Db& db_;
  const NodeId kThisNodeId_;
  mutable std::mutex mutex_;
  std::map<typename MaidAccount::name_type, std::unique_ptr<MaidAccount>> maid_accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_manager/handler-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_HANDLER_H_
