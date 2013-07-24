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

#ifndef MAIDSAFE_VAULT_SYNC_INL_H_
#define MAIDSAFE_VAULT_SYNC_INL_H_

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "boost/optional/optional.hpp"

#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/account_db.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename UnresolvedAction>
bool Recorded(const UnresolvedAction& new_action,
              const UnresolvedAction& existing_action) {
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

template<typename MergePolicy>
typename std::vector<typename MergePolicy::UnresolvedAction::MessageContent>::iterator
    FindInMessages(typename MergePolicy::UnresolvedAction& action, const NodeId& peer_id) {
  return std::find_if(
      std::begin(action.messages_contents),
      std::end(action.messages_contents),
      [&peer_id](const typename MergePolicy::UnresolvedAction::MessageContent& content) {
          return content.peer_id == peer_id;
      });
}

template<typename MergePolicy>
bool IsResolved(const typename MergePolicy::UnresolvedAction& action) {
  return action.messages_contents.size() >
         static_cast<uint32_t>(routing::Parameters::node_group_size / 2);
}

template<typename MergePolicy>
bool IsResolvedOnAllPeers(const typename MergePolicy::UnresolvedAction& action,
                          const NodeId& this_node_id) {
  // If this node is the owner of the last message in the vector, it won't have been
  // synchronised to the peers yet - it's just been added via AddLocalAction.
  return action.messages_contents.size() == routing::Parameters::node_group_size &&
         action.messages_contents.back().peer_id != this_node_id;
}

}  // namespace detail


template<typename MergePolicy>
Sync<MergePolicy>::Sync(typename MergePolicy::Database* database,
                        const NodeId& this_node_id)
    : MergePolicy(database),
      sync_counter_max_(10),  // TODO(dirvine) decide how to decide on this magic number
      this_node_id_(this_node_id) {}

template<typename MergePolicy>
Sync<MergePolicy>::Sync(Sync&& other)
    : MergePolicy(std::forward<MergePolicy>(other)),
      sync_counter_max_(std::move(other.sync_counter_max_)),
      this_node_id_(std::move(other.this_node_id_)) {}

template<typename MergePolicy>
Sync<MergePolicy>& Sync<MergePolicy>::operator=(Sync&& other) {
  // TODO(Fraser#5#): 2013-04-15 - Remove this once MSVC implements default move assignment.
  using std::swap;
  Sync temp(std::move(other));
  swap(*this, temp);
  return *this;
}

template<typename MergePolicy>
std::vector<typename MergePolicy::ResolvedAction>
    Sync<MergePolicy>::AddUnresolvedAction(const typename MergePolicy::UnresolvedAction& action) {
  return AddAction(action, true);
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddLocalAction(const typename MergePolicy::UnresolvedAction& action) {
  AddAction(action, false);
}

template<typename MergePolicy>
std::vector<typename MergePolicy::ResolvedAction>
    Sync<MergePolicy>::AddAction(const typename MergePolicy::UnresolvedAction& action, bool merge) {
  std::vector<typename MergePolicy::ResolvedAction> resolved_actions;
  auto found(std::begin(MergePolicy::unresolved_data_));
  for (;;) {
    found = std::find_if(found,
                         std::end(MergePolicy::unresolved_data_),
                         [&action](const typename MergePolicy::UnresolvedAction &test) {
                             return test.key == action.key &&
                                    test.messages_contents.front().value ==
                                    action.messages_contents.front().value;
                         });

    if (found == std::end(MergePolicy::unresolved_data_)) {
      MergePolicy::unresolved_data_.push_back(action);
      break;
    } else {
      // If merge is false and the action is from this node, we're adding local action, so this
      // shouldn't already exist.
      assert(!merge || action.messages_contents.front().peer_id != this_node_id_);
    }

    if (!detail::Recorded((*found), action)) {
      typename MergePolicy::UnresolvedAction::MessageContent content;
      content.peer_id = action.messages_contents.front().peer_id;
      if (action.messages_contents.front().value)
        content.value = *action.messages_contents.front().value;
      if (action.messages_contents.front().entry_id)
        content.entry_id = *action.messages_contents.front().entry_id;
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

template<typename MergePolicy>
std::vector<typename MergePolicy::ResolvedAction>
    Sync<MergePolicy>::AddAccountTransferRecord(const typename MergePolicy::UnresolvedAction& action,
                                                bool all_account_transfers_received) {
  return AddAction(action, all_account_transfers_received);
}

template<typename MergePolicy>
void Sync<MergePolicy>::ReplaceNode(const NodeId& old_node, const NodeId& new_node) {
  auto itr(std::begin(MergePolicy::unresolved_data_));
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    auto found(detail::FindInMessages<MergePolicy>(*itr, old_node));
    if (found == std::end((*itr).messages_contents)) {
      (*itr).messages_contents.emplace_back();
      (*itr).messages_contents.back().peer_id = new_node;
    } else {
      (*found).peer_id = new_node;
    }

    if (detail::IsResolved<MergePolicy>(*itr)) {
      MergePolicy::Merge(*itr);
      itr = MergePolicy::unresolved_data_.erase(itr);
    } else {
      ++itr;
    }
  }
}

template<typename MergePolicy>
std::vector<typename MergePolicy::UnresolvedAction> Sync<MergePolicy>::GetUnresolvedActions() {
  std::vector<typename MergePolicy::UnresolvedAction> result;
  for (auto& action : MergePolicy::unresolved_data_) {
    if (detail::IsResolvedOnAllPeers<MergePolicy>(action, this_node_id_))
      continue;
    auto found(detail::FindInMessages<MergePolicy>(action, this_node_id_));
    if (found != std::end(action.messages_contents)) {
      // Always move the found message (i.e. this node's message) to the front of the vector.  This
      // serves as an indicator that the action has not been synchronised by this node to the peers
      // if its message is not the first in the vector.  (It's also slightly more efficient to find
      // in future GetUnresolvedActions attempts since we search from begin() to end()).
      result.push_back(action);
      result.back().messages_contents.assign(1, *found);
      std::iter_swap(found, std::begin(action.messages_contents));
    }
  }
  return result;
}

template<typename MergePolicy>
bool Sync<MergePolicy>::CanBeErased(const typename MergePolicy::UnresolvedAction& action) const {
  return action.sync_counter > sync_counter_max_ ||
         detail::IsResolvedOnAllPeers<MergePolicy>(action, this_node_id_);
}

template<typename MergePolicy>
void Sync<MergePolicy>::IncrementSyncAttempts() {
  auto itr = std::begin(MergePolicy::unresolved_data_);
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    assert((*itr).messages_contents.size() <= routing::Parameters::node_group_size);
    ++(*itr).sync_counter;
    if (CanBeErased(*itr))
      itr = MergePolicy::unresolved_data_.erase(itr);
    else
      ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
