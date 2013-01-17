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
#include <vector>


namespace maidsafe {

namespace vault {

template <typename AccountType, typename ModifyPolicy>
class AccountHandler : public ModifyPolicy {
 public:
  AccountHandler(const passport::Pmid& pmid,
                routing::Routing& routing,
                nfs::PublicKeyGetter& public_key_getter,
                const boost::filesystem::path& vault_root_dir) 
      : ModifyPolicy(&mutex_, &accounts_),
        mutex_(), accounts_() {};
  // TODO Check these references are valid and usable in the 'inherited' object
  ~AccountHandler();
  bool AddAccount(const AccountType& account);
  bool DeleteAccount(const typename AccountType::name_type& account);
  // modify here will use the policy class ModifyPolicy members !!
  AccountType GetAccount(const typename AccountType::name_type& account) const;
  std::vector<AccountType::name_type> GetAccountNames() const;

  template <typename Data>
  bool DeleteDataElement(const typename AccountType::name_type& account,
                         const typename Data::name_type& name,
                         int32_t version);
  // This will atomically attempt to modify then add if not found
  // a nullptr can be passed to make this add only
  template <typename Data>
  bool ModifyOrAddDataElement(const typename AccountType::name_type& account,
                              const typename Data::name_type& name,
                              int32_t version,
                              const AccountType::structure account_structure,
                              std::function<void(std::string&)> modify_functor);
 private:
  mutable std::mutex mutex_;
  std::vector<AccountType> accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder/maid_account_holder-inl.h"

#endif  // MAIDSAFE_VAULT_ACCOUNT_HANDLER_H_
