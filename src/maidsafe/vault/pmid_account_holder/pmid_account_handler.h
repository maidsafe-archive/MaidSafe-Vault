/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.pmidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_H_

#include <cstdint>
#include <future>
#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class PmidAccountHandler {
 public:
  explicit PmidAccountHandler();
  // Account operations
  void AddAccount(std::unique_ptr<PmidAccount> pmid_account);
  void DeleteAccount(const PmidName& account_name);
  PmidAccount::DataHolderStatus AccountStatus(const PmidName& account_name) const;
  void SetDataHolderGoingDown(const PmidName& account_name);
  void SetDataHolderDown(const PmidName& account_name);
  void SetDataHolderGoingUp(const PmidName& account_name);
  void SetDataHolderUp(const PmidName& account_name);

  // Sync operations
  std::vector<PmidName> GetAccountNames() const;
  std::vector<PmidName> GetArchivedAccountNames() const;
  PmidAccount::serialised_type GetSerialisedAccount(const PmidName& account_name) const;

  // Data operations
  template<typename Data>
  void Put(const PmidName& account_name, const typename Data::name_type& data_name, int32_t size);
  template<typename Data>
  void Delete(const PmidName& account_name, const typename Data::name_type& data_name);

 private:
  PmidAccountHandler(const PmidAccountHandler&);
  PmidAccountHandler& operator=(const PmidAccountHandler&);
  PmidAccountHandler(PmidAccountHandler&&);
  PmidAccountHandler& operator=(PmidAccountHandler&&);

  const boost::filesystem::path kPmidAccountsRoot_;
  mutable std::mutex mutex_;
  std::map<typename PmidAccount::name_type , std::unique_ptr<PmidAccount>> pmid_accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_account_holder/pmid_account_handler-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HANDLER_H_
