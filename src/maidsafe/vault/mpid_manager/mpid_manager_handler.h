/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_H_

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
  typename std::vector<Account>::iterator FindAccount(
      const typename Account::name_type& account_name);
  typename std::vector<Account>::const_iterator FindAccount(
      const typename Account::name_type& account_name) const;

  mutable std::mutex mutex_;
  std::vector<Account> accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/mpid_manager/mpid_manager_handler-inl.h"

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_H_
