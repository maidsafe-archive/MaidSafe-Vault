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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HANDLER_H_

#include <cstdint>
#include <future>
#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/pmid_account.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class PmidAccountHandler {
 public:
  explicit PmidAccountHandler(const boost::filesystem::path& vault_root_dir);
  // Account operations
  bool AddAccount(std::unique_ptr<PmidAccount> pmid_account);
  bool DeleteAccount(const PmidName& account_name);
  PmidAccount::DataHolderStatus AccountStatus(const PmidName& account_name) const;
  void SetDataHolderGoingDown(const PmidName& account_name);
  void SetDataHolderDown(const PmidName& account_name);
  void SetDataHolderGoingUp(const PmidName& account_name);
  void SetDataHolderUp(const PmidName& account_name);

  // Sync operations
  std::vector<PmidName> GetAccountNames() const;
  std::vector<PmidName> GetArchivedAccountNames() const;
  PmidAccount::serialised_type GetSerialisedAccount(const PmidName& account_name) const;
  std::vector<boost::filesystem::path> GetArchiveFileNames(const PmidName& account_name) const;
  NonEmptyString GetArchiveFile(const PmidName& account_name,
                                const boost::filesystem::path& path) const;
  void PutArchiveFile(const PmidName& account_name,
                      const boost::filesystem::path& path,
                      const NonEmptyString& content);

  void PruneArchivedAccounts(std::function<bool(const PmidName& account_name)> criteria);
  void MoveAccountToArchive(const PmidName& account_name);
  void BringAccountBackFromArchive(const PmidName& account_name);
  void ArchiveRecentData(const PmidName& account_name);

  // Data operations
  template<typename Data>
  std::future<void> PutData(const PmidName& account_name,
                            const typename Data::name_type& data_name,
                            int32_t size);
  template<typename Data>
  std::future<void> DeleteData(const PmidName& account_name,
                               const typename Data::name_type& data_name);

 private:
  PmidAccountHandler(const PmidAccountHandler&);
  PmidAccountHandler& operator=(const PmidAccountHandler&);
  PmidAccountHandler(PmidAccountHandler&&);
  PmidAccountHandler& operator=(PmidAccountHandler&&);

  const boost::filesystem::path kPmidAccountsRoot_;
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<PmidAccount> > pmid_accounts_;
  std::vector<PmidName> archived_accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_account_handler-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HANDLER_H_
