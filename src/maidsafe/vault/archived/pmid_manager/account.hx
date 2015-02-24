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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_ACCOUNT_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_ACCOUNT_H_

#include <cstdint>
#include <deque>
#include <vector>
#include <utility>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/pmid_manager/merge_policy.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {
namespace vault {

class Db;
class AccountDb;

class PmidAccount {
 public:
  typedef PmidName Name;

  enum class PmidNodeStatus : int32_t { kDown, kGoingDown, kUp, kGoingUp };

  PmidAccount(const PmidName& pmid_name, Db &db, const NodeId &this_node_id);
  PmidAccount(const PmidName& pmid_name,
              Db& db,
              const NodeId& this_node_id,
              const NodeId& source_id,
              const serialised_type& serialised_pmid_account_details);

  PmidAccount(PmidAccount&& other);
  PmidAccount& operator=(PmidAccount&& other);

  std::string Serialise(bool include_pmid_record);

  void SetPmidNodeUp() { pmid_node_status_ = PmidNodeStatus::kUp; }
  void SetPmidNodeDown() { pmid_node_status_ = PmidNodeStatus::kDown; }

  void PutData(int32_t size);
  template<typename Data>
  void DeleteData(const typename Data::Name& name);

  bool ApplyAccountTransfer(const NodeId& source_id,
                            const std::string& serialised_pmid_account_details);

  void AddLocalUnresolvedEntry(const PmidManagerUnresolvedEntry& unresolved_entry);
  NonEmptyString GetSyncData();
  void ApplySyncData(const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const NodeId& old_node, const NodeId& new_node);
  void IncrementSyncAttempts();

  PmidManagerMetadata GetMetadata();

  Name name() const;
  PmidNodeStatus pmid_node_status() const;
  int64_t total_data_stored_by_pmids() const;

 private:
  PmidAccount(const PmidAccount&);
  PmidAccount& operator=(const PmidAccount&);

  Name pmid_name_;
  PmidManagerMetadata metadata_;
  PmidNodeStatus pmid_node_status_;
  std::unique_ptr<AccountDb> account_db_;
  Sync<PmidManagerMergePolicy> sync_;
  uint16_t account_transfer_nodes_;
  static const size_t kSyncTriggerCount_;
};


template<typename Data>
void PmidAccount::DeleteData(const typename Data::Name& name) {
  metadata_.stored_count--;
  metadata_.stored_total_size -= sync_.AllowDelete<Data>(name);
}


}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_ACCOUNT_H_
