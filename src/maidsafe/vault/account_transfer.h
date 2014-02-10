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


// ==================== Implementation =============================================================

namespace detail {
// use for both db_action and metadata
template <typename UnresolvedAction>
std::unique_ptr<UnresolvedAction> AddAction(const UnresolvedAction& unresolved_action,
                           std::vector<std::unique_ptr<UnresolvedAction>>& unresolved_actions) {
  auto found = std::find_if(found, std::end(unresolved_actions),
                       [&unresolved_action](const std::unique_ptr<UnresolvedAction>& test) {
                           return ((test->key == unresolved_action.key) &&
                                   (test->action == unresolved_action.action));
                       });
  if (found == std::end(unresolved_actions)) {  // not found

  } else {  // found

  }
}

template <typename UnresolvedAction>
void AddNewUnresolvedAction(const UnresolvedAction& new_action,
    std::vector<std::unique_ptr<UnresolvedAction>>& unresolved_actions) {
}

template <typename UnresolvedAction>
void AppendUnresolvedActionEntry(const UnresolvedAction& new_action,
                                 UnresolvedAction& existing_action,
                                 std::unique_ptr<UnresolvedAction>& resolved_action){
}

}  // namespace detail

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
  std::vector<UnresolvedDbAction> resolved_db_actions;
  for (const auto& unresolved_db_action : unresolved_db_actions) {
    auto resolved(detail::AddAction(unresolved_db_action, unresolved_db_actions_));
    if (resolved) {
      resolved_db_actions.push_back(std::move(*resolved));
    }
  }
  return resolved_db_actions;
}


template <typename UnresolvedDbAction, typename UnresolvedMetadataAction>
    std::pair<UnresolvedMetadataAction, std::vector<UnresolvedDbAction>>
    AccountTransfer<UnresolvedDbAction, UnresolvedMetadataAction>::AddUnresolved(
    const UnresolvedMetadataAction& unresolved_metadata_action,
    std::vector<UnresolvedDbAction>&& unresolved_db_actions) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<UnresolvedDbAction> resolved_db_actions;
  for (const auto& unresolved_db_action : unresolved_db_actions) {
    auto resolved(detail::AddAction(unresolved_db_action, unresolved_db_actions_));
    if (resolved) {
      resolved_db_actions.push_back(std::move(*resolved));
    }
  }
  auto resolved_metadata_action(detail::AddAction(unresolved_metadata_action,
                                                  unresolved_metdata_actions_));
  // FIXME this should not return nullptr if resolved_db_actions has any entry
  // may be need to break metadata actions in furthe small units
  // but this means separate account_transfer objects will be required to merge them.
  // (resulting in locking issues)
  return std::make_pair(resolved_db_actions, resolved_metadata_action);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
