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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HANDLER_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HANDLER_H_

#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

template <typename Account, typename ModifyPolicy>
class AccountHandler : public ModifyPolicy {
 public:
  AccountHandler(const passport::Pmid& pmid, routing::Routing& routing,
                 nfs_client::DataGetter& data_getter,
                 const boost::filesystem::path& vault_root_dir);
  bool AddAccount(const Account& account);
  bool DeleteAccount(const typename Account::Name& account_name);
  // modify here will use the policy class ModifyPolicy members !!
  Account GetAccount(const typename Account::Name& account_name) const;
  std::vector<typename Account::Name> GetAccountNames() const;

  template <typename Data>
  bool DeleteDataElement(const typename Account::Name& account_name,
                         const typename Data::Name& data_name, int32_t data_version);
  // This will atomically attempt to modify then add if not found
  // a nullptr can be passed to make this add only
  template <typename Data>
  bool ModifyOrAddDataElement(const typename Account::Name& account_name,
                              const typename Data::Name& data_name, int32_t data_version,
                              const typename Account::structure& account_structure,
                              std::function<void(std::string&)> modify_functor);

 private:
  typename std::vector<Account>::iterator FindAccount(const typename Account::Name& account_name);
  typename std::vector<Account>::const_iterator FindAccount(
      const typename Account::Name& account_name) const;

  mutable std::mutex mutex_;
  std::vector<Account> accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/mpid_manager/mpid_manager_handler-inl.h"

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HANDLER_H_
