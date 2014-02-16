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


template <typename UnresolvedAction>
class AccountTransfer {
 public:
  AccountTransfer(const NodeId& node_id);
  // For single unresolved action (eg. datamanager key/value and maid manager static metadata)
  std::unique_ptr<UnresolvedAction> AddUnresolvedAction(const UnresolvedAction& unresolved_action);

  // For multiple unresolved actions (eg. maid manager key value pairs)
  std::vector<std::unique_ptr<UnresolvedAction>> AddUnresolvedActions(
      std::vector<UnresolvedDbAction>&& unresolved_actions);

  // To handle double churn situations
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);

 private:
  AccountTransfer(AccountTransfer&&);
  AccountTransfer(const AccountTransfer&);
  AccountTransfer& operator=(AccountTransfer other);

  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<UnresolvedAction>> unresolved_actions_;
  const NodeId kNodeId_;
};


// ==================== Implementation =============================================================

namespace detail {
template <typename UnresolvedAction>
std::unique_ptr<UnresolvedAction> AddAction(const UnresolvedAction& unresolved_action,
    std::vector<std::unique_ptr<UnresolvedAction>>& unresolved_actions) {
  auto found = std::find_if(found, std::end(unresolved_actions),
                       [&unresolved_action](const std::unique_ptr<UnresolvedAction>& test) {
                           return ((test->key == unresolved_action.key) &&
                                   (test->action == unresolved_action.action));
                       });
  if (found == std::end(unresolved_actions)) {  // not found
    // AddNewUnresolvedAction
  } else {  // found
    // AppendUnresolvedActionEntry
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

template <typename UnresolvedAction>
AccountTransfer::AccountTransfer(const NodeId& node_id)
    : mutex_(),
      unresolved_actions_(),
      kNodeId_(node_id) {}

template <typename UnresolvedAction>
std::vector<std::unique_ptr<UnresolvedAction>>
    AccountTransfer<UnresolvedAction>::AddUnresolvedActions(
        std::vector<UnresolvedAction>&& unresolved_actions) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::unique_ptr<UnresolvedAction>> resolved_actions;
  for (const auto& unresolved_action : unresolved_actions) {
    auto resolved(detail::AddAction(unresolved_action, unresolved_actions_));
    if (resolved) {
      resolved_actions.push_back(std::move(*resolved));
    }
  }
  return resolved_actions;
}

template <typename UnresolvedAction>
std::unique_ptr<UnresolvedAction> AccountTransfer<UnresolvedAction>::AddUnresolvedAction(
    const UnresolvedAction& unresolved_action) {
  std::lock_guard<std::mutex> lock(mutex_);
  return detail::AddAction(unresolved_action, unresolved_actions_);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
