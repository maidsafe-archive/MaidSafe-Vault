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

#ifndef MAIDSAFE_VAULT_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_ACCOUNT_HANDLER_H_

#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

template<typename Account, typename ModifyPolicy>
class AccountHandler : public ModifyPolicy {
 public:
  AccountHandler(const passport::Pmid& pmid,
                 routing::Routing& routing,
                 nfs::PublicKeyGetter& public_key_getter,
                 const boost::filesystem::path& vault_root_dir);
  bool AddAccount(const Account& account);
  bool DeleteAccount(const typename Account::name_type& account_name);
  // modify here will use the policy class ModifyPolicy members !!
  Account GetAccount(const typename Account::name_type& account_name) const;
  std::vector<typename Account::name_type> GetAccountNames() const;

  template<typename Data>
  bool DeleteDataElement(const typename Account::name_type& account_name,
                         const typename Data::name_type& data_name,
                         int32_t data_version);
  // This will atomically attempt to modify then add if not found
  // a nullptr can be passed to make this add only
  template<typename Data>
  bool ModifyOrAddDataElement(const typename Account::name_type& account_name,
                              const typename Data::name_type& data_name,
                              int32_t data_version,
                              const typename Account::structure& account_structure,
                              std::function<void(std::string&)> modify_functor);
 private:
  std::vector<Account>::iterator FindAccount(const typename Account::name_type& account_name);
  std::vector<Account>::const_iterator FindAccount(
      const typename Account::name_type& account_name) const;

  mutable std::mutex mutex_;
  std::vector<typename Account> accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/mpid_account_handler-inl.h"

#endif  // MAIDSAFE_VAULT_ACCOUNT_HANDLER_H_
