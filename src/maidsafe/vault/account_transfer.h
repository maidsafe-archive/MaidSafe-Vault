/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"

namespace maidsafe {

namespace vault {


template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
class AccountTransfer {
 public:
  AccountTransfer(const NodeId& node_id);
  // For Data manager like personas
  std::vector<UnresolvedDbAction> AddUnresolved(
          std::vector<UnresolvedDbAction>&& unresolved_db_actions);
  // For MaidManager like personas
  std::pair<UnresolvedMetadataAction, std::vector<UnresolvedDbAction>> AddUnresolved(
      const UnresolvedMetadataAction& metadata_action,
      std::vector<UnresolvedAccountTransferAction>&& unresolved_db_actions);
  // To handle double churn situations
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);

 private:
  AccountTransfer(AccountTransfer&&);
  AccountTransfer(const AccountTransfer&);
  AccountTransfer& operator=(AccountTransfer other);

  std::vector<UnresolvedDbAction> Add(
          std::vector<UnresolvedDbAction>&& unresolved_actions);

  UnresolvedMetadataAction Add(const UnresolvedMetadataAction& unresolved_metadata_action);
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<UnresolvedDbAction>> unresolved_db_actions_;
  std::vector<std::unique_ptr<UnresolvedMetadataAction>> unresolved_metdata_actions_;
  const NodeId kNodeId_;
};


// Implementation


template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
AccountTransfer::AccountTransfer(const NodeId& node_id)
    : mutex_(),
      unresolved_db_actions_(),
      unresolved_metdata_actions_(),
      kNodeId_(node_id) {}



template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
std::vector<UnresolvedDbAction>
    AccountTransfer<UnresolvedDbAction, UnresolvedMetadataAction>::AddUnresolved(
        std::vector<UnresolvedDbAction>&& unresolved_db_actions) {
  std::lock_guard<std::mutex> lock(mutex_);
  return Add(std::move(unresolved_db_actions));
}


template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
    std::pair<UnresolvedMetadataAction, std::vector<UnresolvedDbAction>>
    AccountTransfer<UnresolvedDbAction, UnresolvedMetadataAction>::AddUnresolved(
    const UnresolvedMetadataAction& unresolved_metadata_action,
    std::vector<UnresolvedDbAction>&& unresolved_db_actions) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto resolved_db_actions(Add(std::move(unresolved_db_actions)));
  auto resolved_metadata_action(Add(std::move(unresolved_metadata_action)));
  return std::make_pair(resolved_db_actions, resolved_metadata_action);
}

template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
std::vector<UnresolvedDbAction>
    AccountTransfer<UnresolvedDbAction, UnresolvedMetadataAction>::Add(
        std::vector<UnresolvedDbAction>&& unresolved_db_actions) {
  auto found = std::find_if(found, std::end(unresolved_db_actions_),
                       [&unresolved_db_actions](const std::unique_ptr<UnresolvedAction>& test) {
                           return ((test->key == unresolved_db_actions.key) &&
                                   (test->action == unresolved_db_actions.action));
                       });
  if (found == std::end(unresolved_db_actions_)) {  // not found

  } else {

  }
}


template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
UnresolvedMetadataAction
    AccountTransfer<UnresolvedDbAction, UnresolvedMetadataAction>::Add(
        const UnresolvedMetadataAction& unresolved_metadata_action) {
  auto found = std::find_if(found, std::end(unresolved_metdata_),
                       [&unresolved_metadata_action](const std::unique_ptr<UnresolvedAction>& test) {
                           return ((test->key == unresolved_metdata_actions_.key) &&
                                   (test->action == unresolved_metdata_actions_.action));
                       });
  if (found == std::end(unresolved_metdata_)) {  // not found

  } else {  // found

  }
}





}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
