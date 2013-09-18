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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_INL_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_INL_H_


namespace maidsafe {

namespace vault {

//template<typename Account, typename ModifyPolicy>
//AccountHandler<Account, ModifyPolicy>::AccountHandler(const passport::Pmid& pmid,
//                                                      routing::Routing& routing,
//                                                      nfs::PublicKeyGetter& public_key_getter,
//                                                      const boost::filesystem::path& vault_root_dir)
//// TODO(Fraser#5#): 2013-01-21 - Check these references are valid and usable in ModifyPolicy ctor.
//    : ModifyPolicy(&mutex_, &accounts_),
//      mutex_(),
//      accounts_() {}

//template<typename Account, typename ModifyPolicy>
//bool AccountHandler<Account, ModifyPolicy>::AddAccount(const Account& account) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  if (FindAccount(account.name()) != accounts_.end())
//    return false;

//  accounts_.push_back(account);
//  return true;
//}

//template<typename Account, typename ModifyPolicy>
//bool AccountHandler<Account, ModifyPolicy>::DeleteAccount(
//    const typename Account::Name& account_name) {
//  std::lock_guard<std::mutex> lock(mutex_);
//  auto itr(FindAccount(account_name));
//  if (itr == accounts_.end())
//    return false;

//  accounts_.erase(itr);
//  return true;
//}

//template<typename Account, typename ModifyPolicy>
//Account AccountHandler<Account, ModifyPolicy>::GetAccount(
//    const typename Account::Name& account_name) const {
//  std::lock_guard<std::mutex> lock(mutex_);
//  auto itr(FindAccount(account_name));
//  if (itr == accounts_.end())
//    return Account();

//  return *itr;
//}

//template<typename Account, typename ModifyPolicy>
//  std::vector<typename Account::Name> AccountHandler<Account, ModifyPolicy>::GetAccountNames()
//                                                                                  const {
//  std::vector<typename Account::Name> account_names;
//  std::lock_guard<std::mutex> lock(mutex_);
//  for (auto& account : accounts_)
//    account_names.push_back(account.name());
//  return account_names;
//}

//template<typename Account, typename ModifyPolicy>
//template<typename Data>
//bool AccountHandler<Account, ModifyPolicy>::DeleteDataElement(
//    const typename Account::Name& account_name,
//    const typename Data::Name& data_name,
//    int32_t data_version) {
//}

//template<typename Account, typename ModifyPolicy>
//template<typename Data>
//bool AccountHandler<Account, ModifyPolicy>::ModifyOrAddDataElement(
//    const typename Account::Name& account_name,
//    const typename Data::Name& data_name,
//    int32_t data_version,
//    const typename Account::structure& account_structure,
//    std::function<void(std::string&)> modify_functor) {
//}

//template<typename Account, typename ModifyPolicy>
//typename std::vector<Account>::iterator AccountHandler<Account, ModifyPolicy>::FindAccount(
//    const typename Account::Name& account_name) {
//  return std::find_if(accounts_.begin(),
//                      accounts_.end(),
//                      [&account_name](const Account& account) {
//                        return account_name == account.name();
//                      });
//}

//template<typename Account, typename ModifyPolicy>
//typename std::vector<Account>::const_iterator AccountHandler<Account, ModifyPolicy>::FindAccount(
//    const typename Account::Name& account_name) const {
//  return std::find_if(accounts_.begin(),
//                      accounts_.end(),
//                      [&account_name](const Account& account) {
//                        return account_name == account.name();
//                      });
//}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_ACCOUNT_HANDLER_INL_H_
