/***************************************************************************************************
 *  Copyright 2013 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

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

template<typename UnresolvedEntry>
bool Recorded(const UnresolvedEntry& new_entry,
              const UnresolvedEntry& existing_entry) {
  assert((existing_entry.messages_contents.front().entry_id &&
              new_entry.messages_contents.front().entry_id) ||
         (!existing_entry.messages_contents.front().entry_id &&
              !new_entry.messages_contents.front().entry_id));

  if (new_entry.messages_contents.front().entry_id &&
          existing_entry.messages_contents.front().entry_id) {
    return std::any_of(
                std::begin(existing_entry.messages_contents),
                std::end(existing_entry.messages_contents),
                [&](const typename UnresolvedEntry::MessageContent &test) {
        return test.peer_id == new_entry.messages_contents.front().peer_id &&
               (test.entry_id ?
                   *test.entry_id == *new_entry.messages_contents.front().entry_id :
                   true);
    });
  } else {
    return std::any_of(
        std::begin(existing_entry.messages_contents),
        std::end(existing_entry.messages_contents),
        [&](const typename UnresolvedEntry::MessageContent &test) {
          return test.peer_id == new_entry.messages_contents.front().peer_id;
        });
  }
}

template<typename MergePolicy>
typename std::vector<typename MergePolicy::UnresolvedEntry::MessageContent>::iterator
    FindInMessages(typename MergePolicy::UnresolvedEntry& entry, const NodeId& peer_id) {
  return std::find_if(
      std::begin(entry.messages_contents),
      std::end(entry.messages_contents),
      [&peer_id](const typename MergePolicy::UnresolvedEntry::MessageContent& content) {
          return content.peer_id == peer_id;
      });
}

template<typename MergePolicy>
bool IsResolved(const typename MergePolicy::UnresolvedEntry& entry) {
  return entry.messages_contents.size() > static_cast<uint32_t>(routing::Parameters::node_group_size / 2);
}

template<typename MergePolicy>
bool IsResolvedOnAllPeers(const typename MergePolicy::UnresolvedEntry& entry,
                          const NodeId& this_node_id) {
  // If this node is the owner of the last message in the vector, it won't have been
  // synchronised to the peers yet - it's just been added via AddLocalEntry.
  return entry.messages_contents.size() == routing::Parameters::node_group_size &&
         entry.messages_contents.back().peer_id != this_node_id;
}

}  // namespace detail


template<typename MergePolicy>
Sync<MergePolicy>::Sync(AccountDb* account_db, const NodeId& this_node_id)
    : MergePolicy(account_db),
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
bool Sync<MergePolicy>::AddUnresolvedEntry(const typename MergePolicy::UnresolvedEntry& entry) {
  return AddEntry(entry, true);
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddLocalEntry(const typename MergePolicy::UnresolvedEntry& entry) {
  AddEntry(entry, false);
}

template<typename MergePolicy>
bool Sync<MergePolicy>::AddEntry(const typename MergePolicy::UnresolvedEntry& entry, bool merge) {
  auto found(std::begin(MergePolicy::unresolved_data_));
  for (;;) {
    found = std::find_if(found,
                         std::end(MergePolicy::unresolved_data_),
                         [&entry](const typename MergePolicy::UnresolvedEntry &test) {
                             return test.key == entry.key;
                         });

    if (found == std::end(MergePolicy::unresolved_data_)) {
      MergePolicy::unresolved_data_.push_back(entry);
      break;
    } else {
      // If merge is false and the entry is from this node, we're adding local entry, so this
      // shouldn't already exist.
      assert(!merge || entry.messages_contents.front().peer_id != this_node_id_);
    }

    if (!detail::Recorded((*found), entry)) {
      typename MergePolicy::UnresolvedEntry::MessageContent content;
      content.peer_id = entry.messages_contents.front().peer_id;
      if (entry.messages_contents.front().value)
        content.value = *entry.messages_contents.front().value;
      if (entry.messages_contents.front().entry_id)
        content.entry_id = *entry.messages_contents.front().entry_id;
      (*found).messages_contents.push_back(content);
    }

    if (merge && detail::IsResolved<MergePolicy>(*found)) {
      MergePolicy::Merge(*found);
      return true;
    }

    ++found;
  }
  return false;
}

template<typename MergePolicy>
bool Sync<MergePolicy>::AddAccountTransferRecord(const typename MergePolicy::UnresolvedEntry& entry,
                                                 bool all_account_transfers_received) {
  return AddEntry(entry, all_account_transfers_received);
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
std::vector<typename MergePolicy::UnresolvedEntry> Sync<MergePolicy>::GetUnresolvedData() {
  std::vector<typename MergePolicy::UnresolvedEntry> result;
  for (auto& entry : MergePolicy::unresolved_data_) {
    if (detail::IsResolvedOnAllPeers<MergePolicy>(entry, this_node_id_))
      continue;
    auto found(detail::FindInMessages<MergePolicy>(entry, this_node_id_));
    if (found != std::end(entry.messages_contents)) {
      // Always move the found message (i.e. this node's message) to the front of the vector.  This
      // serves as an indicator that the entry has not been synchronised by this node to the peers
      // if its message is not the first in the vector.  (It's also slightly more efficient to find
      // in future GetUnresolvedData attempts since we search from begin() to end()).
      result.push_back(entry);
      result.back().messages_contents.assign(1, *found);
      std::iter_swap(found, std::begin(entry.messages_contents));
    }
  }
  return result;
}

template<typename MergePolicy>
bool Sync<MergePolicy>::CanBeErased(const typename MergePolicy::UnresolvedEntry& entry) const {
  return entry.sync_counter > sync_counter_max_ ||
         detail::IsResolvedOnAllPeers<MergePolicy>(entry, this_node_id_);
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
