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

#ifndef MAIDSAFE_VAULT_SYNC_H_
#define MAIDSAFE_VAULT_SYNC_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_register_unregister_pmid.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"


namespace maidsafe {

namespace vault {

// The purpose of this class is to ensure enough peers have agreed a given request is valid before
// recording the corresponding unresolved_action to a Persona's database.  This should ensure that all peers
// hold similar, if not identical databases.
template<typename UnresolvedAction>
class Sync {
 public:
  Sync();
  // This is called when receiving a Sync message from a peer or this node. If the
  // unresolved_action becomes resolved then it is returned, otherwise the return is null.
  std::unique_ptr<UnresolvedAction> AddUnresolvedAction(const UnresolvedAction& unresolved_action);
  // This is called directly once an unresolved_action has been decided as valid in the Service, but before
  // syncing the unresolved unresolved_action to the peers.  This won't resolve the unresolved_action (even if it's the
  // last one we're waiting for) so that 'GetUnresolvedActions()' will return this one, allowing us
  // to then sync it to our peers.
  void AddLocalAction(const UnresolvedAction& unresolved_action);
  // This is called if, during an ongoing account transfer, another churn event occurs.  The
  // old node's ID is replaced throughout the list of unresolved actions with the new node's ID.
  // The new node's ID is also applied to any actions which didn't contain the old one, in the
  // expectation that the old node would have eventually supplied the message.
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);
  // This returns all unresoved actions containing this node's ID.  Each returned unresolved_action is provided
  // with just this node's ID inserted, even if the master copy has several other peers' IDs.
  std::vector<std::unique_ptr<UnresolvedAction>> GetUnresolvedActions();
  // Calling this will increment the sync counter and delete actions that reach the
  // 'kSyncCounterMax_' limit.  Actions which are resolved by all peers (i.e. have 4 messages) are
  // also pruned here.
  void IncrementSyncAttempts();

 private:
  Sync(Sync&&);
  Sync(const Sync&);
  Sync& operator=(Sync other);
  std::unique_ptr<UnresolvedAction> AddAction(const UnresolvedAction& unresolved_action,
                                              bool merge);
  bool CanBeErased(const UnresolvedAction& unresolved_action) const;

  std::mutex mutex_;
  std::vector<std::unique_ptr<UnresolvedAction>> unresolved_actions_;
  static const int32_t kSyncCounterMax_ = 10;  // TODO(dirvine) decide how to decide on this number.
};



// ==================== Implementation =============================================================
namespace detail {

template<typename UnresolvedAction>
bool IsFromThisNode(const UnresolvedAction& unresolved_action) {
  return !unresolved_action.this_node_and_entry_id.first.IsZero();
}

template<typename UnresolvedAction>
bool IsRecorded(const UnresolvedAction& new_action, const UnresolvedAction& existing_action) {
  assert(IsFromThisNode(new_action) ? new_action.peer_and_entry_ids.empty() :
                                      new_action.peer_and_entry_ids.size() == 1U);

  if (IsFromThisNode(new_action)) {
    assert(new_action.this_node_and_entry_id.first == existing_action.this_node_and_entry_id.first);
    return new_action.this_node_and_entry_id.second ==
           existing_action.this_node_and_entry_id.second;
  }

  return std::any_of(std::begin(existing_action.peer_and_entry_ids),
                     std::end(existing_action.peer_and_entry_ids),
                     [&new_action](const std::pair<NodeId, int32_t>& test) {
      return test == new_action.peer_and_entry_ids.front();
  });
}

template<typename UnresolvedAction>
bool IsResolved(const UnresolvedAction& unresolved_action) {
  return unresolved_action.peer_and_entry_ids.size() + (IsFromThisNode(unresolved_action) ? 1 : 0) >
         static_cast<uint32_t>(routing::Parameters::node_group_size / 2);
}

template<typename UnresolvedAction>
bool IsResolvedOnAllPeers(const UnresolvedAction& unresolved_action) {
  return unresolved_action.peer_and_entry_ids.size() == routing::Parameters::node_group_size - 1U &&
         unresolved_action.sent_to_peers;
}

}  // namespace detail


template<typename UnresolvedAction>
Sync<UnresolvedAction>::Sync() : mutex_(), unresolved_actions_() {}

template<typename UnresolvedAction>
std::unique_ptr<UnresolvedAction> Sync<UnresolvedAction>::AddUnresolvedAction(
    const UnresolvedAction& unresolved_action) {
  return AddAction(unresolved_action, true);
}

template<typename UnresolvedAction>
void Sync<UnresolvedAction>::AddLocalAction(const UnresolvedAction& unresolved_action) {
  AddAction(unresolved_action, false);
}

template<typename UnresolvedAction>
std::unique_ptr<UnresolvedAction> Sync<UnresolvedAction>::AddAction(
    const UnresolvedAction& unresolved_action,
    bool merge) {
  std::unique_ptr<UnresolvedAction> resolved_action;
  auto found(std::begin(unresolved_actions_));
  for (;;) {
    found = std::find_if(found,
                         std::end(unresolved_actions_),
                         [&unresolved_action](const std::unique_ptr<UnresolvedAction> &test) {
                           return test->key == unresolved_action.key;
                         });

    if (found == std::end(unresolved_actions_)) {
      std::unique_ptr<UnresolvedAction>
          unresolved_action_ptr(new UnresolvedAction(unresolved_action));
      unresolved_actions_.push_back(std::move(unresolved_action_ptr));
      break;
    } else {
      // If merge is false and the unresolved_action is from this node, we're adding local
      // unresolved_action, so this shouldn't already exist.
      assert(!merge || !detail::IsFromThisNode(unresolved_action));
    }

    if (!detail::IsRecorded(unresolved_action, (*found))) {
      if (detail::IsFromThisNode(unresolved_action)) {
        found->this_node_and_entry_id = unresolved_action.this_node_and_entry_id;
      } else {
        found->peer_and_entry_ids.push_back(unresolved_action.peer_and_entry_ids.front());
      }
    }

    if (merge && detail::IsResolved(*found)) {
      resolved_action.reset(new UnresolvedAction(*found));
      break;
    }

    ++found;
  }

  return std::move(resolved_action);
}

template<typename UnresolvedAction>
std::vector<std::unique_ptr<UnresolvedAction>> Sync<UnresolvedAction>::GetUnresolvedActions() {
  std::vector<std::unique_ptr<UnresolvedAction>> result;
  for (auto& unresolved_action : unresolved_actions_) {
    if (detail::IsResolvedOnAllPeers(*unresolved_action))
      continue;
    if (detail::IsFromThisNode(*unresolved_action)) {
      unresolved_action->sent_to_peers = true;
      std::unique_ptr<UnresolvedAction>
        unresolved_action_ptr(new UnresolvedAction(*unresolved_action));
      result.push_back(std::move(unresolved_action_ptr));
    }
  }
  return result;
}

template<typename UnresolvedAction>
bool Sync<UnresolvedAction>::CanBeErased(const UnresolvedAction& unresolved_action) const {
  return unresolved_action.sync_counter > kSyncCounterMax_ ||
         detail::IsResolvedOnAllPeers(unresolved_action);
}

template<typename UnresolvedAction>
void Sync<UnresolvedAction>::IncrementSyncAttempts() {
  auto itr = std::begin(unresolved_actions_);
  while (itr != std::end(unresolved_actions_)) {
    assert((*itr)->peer_and_entry_ids.size() < routing::Parameters::node_group_size);
    ++(*itr)->sync_counter;
    if (CanBeErased(**itr))
      itr = unresolved_actions_.erase(itr);
    else
      ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_H_
