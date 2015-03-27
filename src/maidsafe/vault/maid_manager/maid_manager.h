/*  Copyright 2015 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/passport/types.h"
#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"
#include "maidsafe/vault/maid_manager/account.h"


namespace maidsafe {
namespace vault {

template <typename Facade>
class MaidManager {
 public:
  using AccountName = MaidManagerAccount::AccountName;

  MaidManager() = default;

  void HandleCreateAccount(const passport::PublicMaid& public_maid,
                           const passport::PublicAnmaid& public_anmaid,
                           int64_t space_offered = std::numeric_limits<uint64_t>().max());
  template <typename Data>
  routing::HandlePutPostReturn HandlePut(const routing::SourceAddress& address, const Data& data);
  template <typename Data>
  void HandlePutResponse(const AccountName& name, const Data& data);
  void HandleChurn(const routing::CloseGroupDifference& close_group_difference);

  bool HasAccount(const AccountName& account_name);

 private:
  std::mutex accounts_mutex_;
  std::set<MaidManagerAccount> accounts_;
};

template <typename Facade>
void MaidManager<Facade>::HandleCreateAccount(const passport::PublicMaid& public_maid,
                                              const passport::PublicAnmaid& public_anmaid,
                                              int64_t space_offered) {
  {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    if (std::any_of(std::begin(accounts_), std::end(accounts_),
            [=](const MaidManagerAccount& account) {
              return account.name() == public_maid.name();
            }))
      BOOST_THROW_EXCEPTION(MakeError(VaultErrors::account_already_exists));
    accounts_.insert(MaidManagerAccount(public_maid.name(), 0, space_offered));
  }

  auto remove_account([=]{
    {
      std::lock_guard<std::mutex> lock(accounts_mutex_);
      auto it(std::find_if(std::begin(accounts_), std::end(accounts_),
          [=](const MaidManagerAccount& account) {
            return account.name() == public_maid.name();
          }));
      if (it != std::end(accounts_))
        accounts_.erase(it);
    }
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));
  });

  static_cast<Facade*>(this)->template
    Put<passport::PublicMaid>(routing::Address(public_maid.name()), public_maid,
      [=](maidsafe_error error) {
        if (error.code() != make_error_code(CommonErrors::success))
          remove_account();
      });

  static_cast<Facade*>(this)->template
    Put<passport::PublicAnmaid>(routing::Address(public_anmaid.name()), public_anmaid,
      [=](maidsafe_error error) {
        if (error.code() != make_error_code(CommonErrors::success))
          remove_account();
      });
}

template <typename Facade> template <typename Data>
routing::HandlePutPostReturn MaidManager<Facade>::HandlePut(
    const routing::SourceAddress& source_address, const Data& data) {
  {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    auto it(std::find_if(std::begin(accounts_), std::end(accounts_),
      [=](const MaidManagerAccount& account) {
        return account.name().string() == source_address.node_address.data.string();
      }));

    if (it == std::end(accounts_))
      return boost::make_unexpected(maidsafe_error(VaultErrors::no_such_account));
    if (it->AllowPut(data) == MaidManagerAccount::Status::kNoSpace)
      return boost::make_unexpected(maidsafe_error(CommonErrors::cannot_exceed_limit));

    auto account(*it);
    account.PutData(MaidManagerAccount::kWeight * Serialise(data).size());
    it = accounts_.erase(it);
    accounts_.insert(it, account);
  }

  std::vector<routing::DestinationAddress> result;
  result.push_back(std::make_pair(routing::Destination(routing::Address(data.Name())),
                                  boost::optional<routing::ReplyToAddress>()));
  return routing::HandlePutPostReturn(result);
}

template <typename Facade>
void MaidManager<Facade>::HandleChurn(
    const routing::CloseGroupDifference& close_group_difference) {
  std::lock_guard<std::mutex> lock(accounts_mutex_);
  std::vector<routing::Address> old_accounts(close_group_difference.first);
  for (auto& old_account : old_accounts) {
    auto it(std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const MaidManagerAccount& account) {
          return old_account.string() == account.name().value.string();
        }));
    if (it != std::end(accounts_))
      accounts_.erase(it);
  }
  std::vector<routing::Address> send_accounts(close_group_difference.second);
  for (auto& send_account : send_accounts) {
    auto it(std::find_if(std::begin(accounts_), std::end(accounts_),
        [=](const MaidManagerAccount& account) {
          return send_account.string() == account.name().value.string();
        }));
    if (it != std::end(accounts_)) {
      // TODO(team) send account
      accounts_.erase(it);
    }
  }
}

template <typename Facade>
bool MaidManager<Facade>::HasAccount(const AccountName& account_name) {
  std::lock_guard<std::mutex> lock(accounts_mutex_);
  return std::any_of(std::begin(accounts_), std::end(accounts_),
    [=](const MaidManagerAccount& account) {
      return account.name() == account_name;
    });
}

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
