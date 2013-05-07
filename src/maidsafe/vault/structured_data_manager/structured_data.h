/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_ACCOUNT_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_ACCOUNT_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/structured_data_manager/structured_data_merge_policy.h"


namespace maidsafe {

namespace vault {

class Db;
class AccountDb;

namespace protobuf { class MaidAccount; }

namespace test {

class MaidAccountHandlerTest;

template<typename Data>
class MaidAccountHandlerTypedTest;

}  // namespace test


class StructuredDataAccount {
 public:
  typedef MaidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedStructuredDataAccountTag> serialised_type;

  StructuredDataAccount(const MaidName& maid_name, Db& db, const NodeId& this_node_id);
  // For creating new account via account transfer
  StructuredDataAccount(const MaidName& maid_name,
              Db& db,
              const NodeId& this_node_id,
              const NodeId& source_id,
              const serialised_type& serialised_maid_account_details);

  StructuredDataAccount(StructuredDataAccount&& other);
  StructuredDataAccount& operator=(StructuredDataAccount&& other);

  serialised_type Serialise();

  bool ApplyAccountTransfer(const NodeId& source_id,
                            const serialised_type& serialised_maid_account_details);

  // headers and unresolved data
  void AddLocalUnresolvedEntry(const MaidAccountUnresolvedEntry& unresolved_entry);
  NonEmptyString GetSyncData();
  void ApplySyncData(const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const NodeId& old_node, const NodeId& new_node);
  void IncrementSyncAttempts();

  name_type name() const { return maid_name_; }


 private:
  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);

  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  name_type maid_name_;
  std::vector<PmidTotals> pmid_totals_;
  int64_t total_claimed_available_size_by_pmids_, total_put_data_;
  std::unique_ptr<AccountDb> account_db_;
  Sync<MaidAccountMergePolicy> sync_;
  uint16_t account_transfer_nodes_;
  static const size_t kSyncTriggerCount_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_ACCOUNT_H_
