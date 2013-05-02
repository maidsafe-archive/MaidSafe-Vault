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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_account_holder/maid_account.h"



namespace maidsafe {

namespace vault {

namespace test { class MaidAccountHandlerTest; }


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
  void UpdatePmidTotals(const MaidName& account_name, const PmidTotals& pmid_totals);
  void AddLocalEntry(const MaidName& account_name,
                     UnresolvedData<nfs::Persona::kMaidAccountHolder>& entry);
  // Sync operations
  std::vector<MaidName> GetAccountNames() const;
  MaidAccount::serialised_type GetSerialisedAccount(const MaidName& account_name) const;
  NonEmptyString GetSyncData(const MaidName& account_name);
  void ApplySyncData(const MaidName& account_name,
                     const NodeId& source_id,
                     const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const MaidName& account_name,
                             const NodeId& old_node,
                             const NodeId& new_node);

  typedef std::true_type RequireAccount;
  typedef std::false_type RequireNoAccount;

  // Data operations
  template<typename Data>
  void PutData(const MaidName& account_name,
               const typename Data::name_type& data_name,
               int32_t cost,
               RequireAccount);
  // this will create an account on storing a MAID
  template<typename Data>
  void PutData(const MaidName& account_name,
               const typename Data::name_type& data_name,
               int32_t cost,
               RequireNoAccount);  // only Maid and AnMaid
  MaidAccount::Status AllowPut(const MaidName& account_name, int32_t cost) const;

  template<typename Data>
  void DeleteData(const MaidName& account_name, const typename Data::name_type& data_name);

  friend class test::MaidAccountHandlerTest;

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

#include "maidsafe/vault/maid_account_holder/maid_account_handler-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_
