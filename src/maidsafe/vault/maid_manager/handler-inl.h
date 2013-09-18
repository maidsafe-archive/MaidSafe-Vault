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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_ACCOUNT_HANDLER_INL_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_ACCOUNT_HANDLER_INL_H_

#include "maidsafe/passport/types.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace detail {

template<typename Data>
struct AccountRequired : std::true_type {};

template<>
struct AccountRequired<passport::Anmaid> : std::false_type {};

template<>
struct AccountRequired<passport::Maid> : std::false_type {};

}  // namespace detail


template<typename Data>
void MaidAccountHandler::CreateAccount(const MaidName& account_name, AllowedAccountCreationType) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<MaidAccount> account(new MaidAccount(account_name, db_, kThisNodeId_));
  // if account exists, this is a no-op (allow Maid and Anmaid to be stored several times)
  maid_accounts_.insert(std::move(std::make_pair(account_name, std::move(account))));
}

template<typename Data>
void MaidAccountHandler::DeleteData(const MaidName& account_name,
                                    const typename Data::Name& data_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  maid_accounts_.at(account_name)->DeleteData<Data>(data_name);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_ACCOUNT_HANDLER_INL_H_
