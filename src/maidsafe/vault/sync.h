/* Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_SYNC_H_
#define MAIDSAFE_VAULT_SYNC_H_

#include <cstdint>
#include <mutex>
#include <vector>

#include "maidsafe/common/node_id.h"




namespace maidsafe {

namespace vault {

// The purpose of this class is to ensure enough peers have agreed a given request is valid before
// recording the corresponding unresolved_action to a Persona's database.  This should ensure that all peers
// hold similar, if not identical databases.
template<typename UnresolvedAction>
class Sync {
 public:
  explicit Sync(const NodeId& this_node_id);
  // This is called when receiving a Sync message from a peer or this node. If the
  // unresolved_action becomes resolved then size() >= routing::Parameters::node_group_size -1
  std::vector<UnresolvedAction> AddUnresolvedAction(const UnresolvedAction& unresolved_action);
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
  std::vector<UnresolvedAction> GetUnresolvedActions();
  // Calling this will increment the sync counter and delete actions that reach the
  // 'sync_counter_max_' limit.  Actions which are resolved by all peers (i.e. have 4 messages) are
  // also pruned here.
  void IncrementSyncAttempts();

 private:
  Sync(Sync&&);
  Sync(const Sync&);
  Sync& operator=(Sync other);
  std::vector<UnresolvedAction> AddAction(const UnresolvedAction& unresolved_action, bool merge);
  //template <typename Data>
  //bool AddAction(const UnresolvedAction& unresolved_action, bool merge);
  bool CanBeErased(const UnresolvedAction& unresolved_action) const;

  const NodeId kThisNodeId_;
  std::mutex mutex_;
  std::vector<UnresolvedAction> unresolved_actions_;
  static const int32_t kSyncCounterMax_ = 10;  // TODO(dirvine) decide how to decide on this number.
};



// ==================== Implementation =============================================================
namespace detail {

template<typename UnresolvedAction>
bool Recorded(const UnresolvedAction& new_action, const UnresolvedAction& existing_action) {
  assert((existing_action.messages_contents.front().entry_id &&
              new_action.messages_contents.front().entry_id) ||
         (!existing_action.messages_contents.front().entry_id &&
              !new_action.messages_contents.front().entry_id));

  if (new_action.messages_contents.front().entry_id &&
          existing_action.messages_contents.front().entry_id) {
    return std::any_of(
                std::begin(existing_action.messages_contents),
                std::end(existing_action.messages_contents),
                [&](const typename UnresolvedAction::MessageContent &test) {
        return test.peer_id == new_action.messages_contents.front().peer_id &&
               (test.entry_id ?
                   *test.entry_id == *new_action.messages_contents.front().entry_id :
                   true);
    });
  } else {
    return std::any_of(
        std::begin(existing_action.messages_contents),
        std::end(existing_action.messages_contents),
        [&](const typename UnresolvedAction::MessageContent &test) {
          return test.peer_id == new_action.messages_contents.front().peer_id;
        });
  }
}

//template<typename UnresolvedAction>
//typename std::vector<UnresolvedAction::MessageContent>::iterator FindInMessages(
//    UnresolvedAction& unresolved_action,
//    const NodeId& peer_id) {
//  return std::find_if(
//      std::begin(unresolved_action.messages_contents),
//      std::end(unresolved_action.messages_contents),
//      [&peer_id](const UnresolvedAction::MessageContent& content) {
//          return content.peer_id == peer_id;
//      });
//}

template<typename UnresolvedAction>
bool IsResolved(const UnresolvedAction& unresolved_action) {
  return unresolved_action.messages_contents.size() >
         static_cast<uint32_t>(routing::Parameters::node_group_size / 2);
}

template<typename UnresolvedAction>
bool IsResolvedOnAllPeers(const UnresolvedAction& unresolved_action,
                          const NodeId& this_node_id) {
  // If this node is the owner of the last message in the vector, it won't have been
  // synchronised to the peers yet - it's just been added via AddLocalAction.
  return unresolved_action.messages_contents.size() == routing::Parameters::node_group_size &&
         unresolved_action.messages_contents.back().peer_id != this_node_id;
}

}  // namespace detail


template<typename UnresolvedAction>
Sync<UnresolvedAction>::Sync(const NodeId& this_node_id)
    : kThisNodeId_(this_node_id),
      mutex_(),
      unresolved_actions_() {}

template<typename UnresolvedAction>
std::vector<UnresolvedAction> Sync<UnresolvedAction>::AddUnresolvedAction(
    const UnresolvedAction& unresolved_action) {
  return AddAction(unresolved_action, true);
}

template<typename UnresolvedAction>
void Sync<UnresolvedAction>::AddLocalAction(const UnresolvedAction& unresolved_action) {
  AddAction(unresolved_action, false);
}

template<typename UnresolvedAction>
std::vector<UnresolvedAction> Sync<UnresolvedAction>::AddAction(
    const UnresolvedAction& unresolved_action, bool merge) {
  std::vector<UnresolvedAction> resolved_actions;
  auto found(std::begin(unresolved_actions_));
  for (;;) {
    found = std::find_if(found,
                         std::end(unresolved_actions_),
                         [&unresolved_action](const UnresolvedAction &test) {
                             return test.key == unresolved_action.key &&
                                    test.messages_contents.front().value ==
                                    unresolved_action.messages_contents.front().value;
                         });

    if (found == std::end(unresolved_actions_)) {
      unresolved_actions_.push_back(unresolved_action);
      break;
    } else {
      // If merge is false and the unresolved_action is from this node, we're adding local
      // unresolved_action, so this shouldn't already exist.
      assert(!merge || unresolved_action.messages_contents.front().peer_id != this_node_id_);
    }

    if (!detail::Recorded((*found), unresolved_action)) {
      UnresolvedAction::MessageContent content;
      content.peer_id = unresolved_action.messages_contents.front().peer_id;
      if (unresolved_action.messages_contents.front().value)
        content.value = *unresolved_action.messages_contents.front().value;
      if (unresolved_action.messages_contents.front().entry_id)
        content.entry_id = *unresolved_action.messages_contents.front().entry_id;
      (*found).messages_contents.push_back(content);
    }

    if (merge && detail::IsResolved<MergePolicy>(*found)) {
      MergePolicy::Merge(*found);
      resolved_actions.push_back(*found);
    }

    ++found;
  }
  return resolved_actions;
}

template<typename UnresolvedAction>
std::vector<UnresolvedAction> Sync<UnresolvedAction>::GetUnresolvedActions() {
  std::vector<UnresolvedAction> result;
  for (auto& unresolved_action : unresolved_actions_) {
    if (detail::IsResolvedOnAllPeers<MergePolicy>(unresolved_action, this_node_id_))
      continue;
    auto found(detail::FindInMessages<MergePolicy>(unresolved_action, this_node_id_));
    if (found != std::end(unresolved_action.messages_contents)) {
      // Always move the found message (i.e. this node's message) to the front of the vector.  This
      // serves as an indicator that the unresolved_action has not been synchronised by this node to the peers
      // if its message is not the first in the vector.  (It's also slightly more efficient to find
      // in future GetUnresolvedActions attempts since we search from begin() to end()).
      result.push_back(unresolved_action);
      result.back().messages_contents.assign(1, *found);
      std::iter_swap(found, std::begin(unresolved_action.messages_contents));
    }
  }
  return result;
}

template<typename UnresolvedAction>
bool Sync<UnresolvedAction>::CanBeErased(const UnresolvedAction& unresolved_action) const {
  return unresolved_action.sync_counter > sync_counter_max_ ||
         detail::IsResolvedOnAllPeers<MergePolicy>(unresolved_action, this_node_id_);
}

template<typename UnresolvedAction>
void Sync<UnresolvedAction>::IncrementSyncAttempts() {
  auto itr = std::begin(unresolved_actions_);
  while (itr != std::end(unresolved_actions_)) {
    assert((*itr).messages_contents.size() <= routing::Parameters::node_group_size);
    ++(*itr).sync_counter;
    if (CanBeErased(*itr))
      itr = unresolved_actions_.erase(itr);
    else
      ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_H_
