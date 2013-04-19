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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_account_holder/pmid_record.h"
#include "maidsafe/vault/maid_account_holder/maid_account_merge_policy.h"


namespace maidsafe {

namespace vault {

class Db;
class AccountDb;

namespace test {

class MaidAccountHandlerTest;

template<typename Data>
class MaidAccountHandlerTypedTest;

}  // namespace test


struct PmidTotals {
  PmidTotals();
  PmidTotals(const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in,
             const PmidRecord& pmid_record_in);
  PmidTotals(const PmidTotals& other);
  PmidTotals(PmidTotals&& other);
  PmidTotals& operator=(PmidTotals other);

  nfs::PmidRegistration::serialised_type serialised_pmid_registration;
  PmidRecord pmid_record;
};

class MaidAccount {
 public:
  enum class Status { kOk, kLowSpace };
  typedef MaidName name_type;

  struct State {
    State();
    State(const State& other);
    State(State&& other);
    State& operator=(State other);

    uint64_t id;
    std::vector<PmidTotals> pmid_totals;
    int64_t total_claimed_available_size_by_pmids, total_put_data;
  };

  // For client adding new account
  MaidAccount(const MaidName& maid_name, Db& db, const NodeId& this_node_id);
  // For creating new account via account transfer
  MaidAccount(const MaidName& maid_name, const State& state, Db& db, const NodeId& this_node_id);

  MaidAccount(MaidAccount&& other);
  MaidAccount& operator=(MaidAccount&& other);
//  void ArchiveToDisk() const;

  void RegisterPmid(const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  void UpdatePmidTotals(const PmidTotals& pmid_totals);

  // Overwites existing state.  Used if this account is out of date (e.g was archived then restored)
//  void ApplySyncInfo(const State& state);
  // Replaces any existing files.  Used if this account is out of date (e.g was archived then
  // restored)
//  void ApplyTransferredFiles(const boost::filesystem::path& transferred_files_dir);
//  NonEmptyString GetArchiveFile(const boost::filesystem::path& filename) const;

  // Returns file contents and current state id.
  std::pair<NonEmptyString, uint64_t> GetArchiveFile(int32_t index) const;

  State GetState() const;

  // Returns all recent ops from class vector
//  std::vector<DiskBasedStorage::RecentOperation> GetRecentOps() const;
//  void ReinstateUnmergedRecentOps(
//      const std::vector<DiskBasedStorage::RecentOperation>& unmerged_recent_ops);
  // Applies this list and removes it from current ops.
//  void ApplyRecentOps(const std::vector<DiskBasedStorage::RecentOperation>& confirmed_recent_ops);

  // This offers the strong exception guarantee
  template<typename Data>
  Status PutData(const typename Data::name_type& name, int32_t cost);
  // This offers the strong exception guarantee
  template<typename Data>
  void DeleteData(const typename Data::name_type& name);

  template<typename Data>
  void Adjust(const typename Data::name_type& name, int32_t cost);

  name_type name() const { return maid_name_; }

  friend class test::MaidAccountHandlerTest;
  template<typename Data>
  friend class test::MaidAccountHandlerTypedTest;

 private:
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidAccountTag> serialised_type;
  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);

  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  template<typename Data>
  Status DoPutData(const typename Data::name_type& name, int32_t cost);

  name_type maid_name_;
  State state_;
  std::unique_ptr<AccountDb> account_db_;
  Sync<MaidAccountMergePolicy> sync_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder/maid_account-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
