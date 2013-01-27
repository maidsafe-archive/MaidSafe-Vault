/***************************************************************************************************
 *  Copyright 2012 PmidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of PmidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.pmidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of PmidSafe.net.                                  *
 **************************************************************************************************/

#ifndef pmidSAFE_VAULT_ACCOUNT_HANDLER_H_
#define pmidSAFE_VAULT_ACCOUNT_HANDLER_H_

#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/vault/types.h"


namespace pmidsafe {

namespace vault {

class PmidAccountHandler {
 public:
  explicit PmidAccountHandler(const boost::filesystem::path& vault_root_dir);
  // Account operations
  bool AddAccount(const PmidAccount& pmid_account);
  bool DeleteAccount(const PmidName& account_name);

  // Sync operations
  std::vector<PmidName> GetAccountNames() const;
  PmidAccount::serialised_type GetSerialisedAccount(const PmidName& account_name) const;
  std::vector<boost::filesystem::path> GetArchiveFileNames(const PmidName& account_name) const;
  NonEmptyString GetArchiveFile(const PmidName& account_name,
                                const boost::filesystem::path& path) const;
  void PutArchiveFile(const PmidName& account_name,
                      const boost::filesystem::path& path,
                      const NonEmptyString& content);

  // Data operations
  template<typename Data>
  void PutData(const PmidName& account_name,
               const typename Data::name_type& data_name,
               int32_t size);
  template<typename Data>
  void DeleteData(const PmidName& account_name, const typename Data::name_type& data_name);
  template<typename Data>

 private:
  PmidAccountHandler(const PmidAccountHandler&);
  PmidAccountHandler& operator=(const PmidAccountHandler&);
  PmidAccountHandler(PmidAccountHandler&&);
  PmidAccountHandler& operator=(PmidAccountHandler&&);
  const boost::filesystem::path kPmidAccountsRoot_;
  mutable std::mutex mutex_;
  std::vector<PmidAccount> pmid_accounts_;
};

}  // namespace vault

}  // namespace pmidsafe

#include "pmidsafe/vault/account_handler-inl.h"

#endif  // pmidSAFE_VAULT_ACCOUNT_HANDLER_H_
