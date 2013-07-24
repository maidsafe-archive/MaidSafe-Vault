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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/helpers.h"
#include "maidsafe/vault/maid_manager/merge_policy.h"
#include "maidsafe/vault/sync.h"

namespace maidsafe {

namespace vault {

class Db;
class AccountDb;
struct PmidManagerMetadata;

//namespace protobuf { class MaidManager; }

namespace test {

class MaidManagerHandlerTest;

template<typename Data>
class MaidManagerHandlerTypedTest;

}  // namespace test


class MaidAccount {
 public:
  enum class Status { kOk, kLowSpace, kNoSpace };
  typedef MaidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidManagerTag> serialised_type;

  // For client adding new account
  MaidAccount(const MaidName& maid_name, Db& db, const NodeId& this_node_id);
  // For creating new account via account transfer
  MaidAccount(const MaidName& maid_name,
              Db& db,
              const NodeId& this_node_id,
              const NodeId& source_id,
              const serialised_type& serialised_maid_account_details);

  MaidAccount(MaidAccount&& other);
  MaidAccount& operator=(MaidAccount&& other);

  serialised_type Serialise();

  bool ApplyAccountTransfer(const NodeId& source_id,
                            const serialised_type& serialised_maid_account_details);
  void RegisterPmid(const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  std::vector<PmidName> GetPmidNames() const;
  void UpdatePmidTotals(const PmidManagerMetadata& pmid_record);

  // headers and unresolved data
  void AddLocalUnresolvedAction(const MaidManagerUnresolvedAction& unresolved_action);
  NonEmptyString GetSyncData();
  void ApplySyncData(const NonEmptyString& serialised_unresolved_actions);
  void ReplaceNodeInSyncList(const NodeId& old_node, const NodeId& new_node);
  void IncrementSyncAttempts();

  Status AllowPut(int32_t cost) const;
  // This offers the strong exception guarantee
  template<typename Data>
  void DeleteData(const typename Data::name_type& name) {
    total_put_data_ -= sync_.AllowDelete<Data>(name);
  }
  name_type name() const { return maid_name_; }

  friend class test::MaidManagerHandlerTest;
  template<typename Data>
  friend class test::MaidManagerHandlerTypedTest;

 private:
  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);

  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  name_type maid_name_;
  std::vector<PmidTotals> pmid_totals_;
  int64_t total_claimed_available_size_by_pmids_, total_put_data_;
  std::unique_ptr<AccountDb> account_db_;
  Sync<MaidManagerMergePolicy> sync_;
  uint16_t account_transfer_nodes_;
  static const size_t kSyncTriggerCount_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_
