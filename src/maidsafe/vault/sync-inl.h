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
  return entry.messages_contents.size() >
         static_cast<uint32_t>(routing::Parameters::node_group_size / 2);
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
Sync<MergePolicy>::Sync(typename MergePolicy::Database* database, const NodeId& this_node_id)
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
boost::optional<typename MergePolicy::ResolvedEntry> Sync<MergePolicy>::AddUnresolvedEntry(
    const typename MergePolicy::UnresolvedEntry& entry) {
  return AddEntry(entry, true);
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddLocalEntry(const typename MergePolicy::UnresolvedEntry& entry) {
  AddEntry(entry, false);
}

template<typename MergePolicy>
boost::optional<typename MergePolicy::ResolvedEntry> Sync<MergePolicy>::AddEntry(
    const typename MergePolicy::UnresolvedEntry& entry,
    bool merge) {
  boost::optional<typename MergePolicy::ResolvedEntry> resolved_entry;
  auto found = std::find_if(std::begin(MergePolicy::unresolved_data_),
                            std::end(MergePolicy::unresolved_data_),
                            [&entry](const typename MergePolicy::UnresolvedEntry &test) {
                                return test.key == entry.key;
                            });

  if (found == std::end(MergePolicy::unresolved_data_)) {
    MergePolicy::unresolved_data_.push_back(entry);
    return resolved_entry;
  }

  // If merge is false and the entry is from this node, we're adding local entry, so this
  // shouldn't already exist.
  assert(!merge || entry.messages_contents.front().peer_id != this_node_id_);

  if (!detail::Recorded(*found, entry)) {
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
    resolved_entry = *found;
  }

  return resolved_entry;
}

template<typename MergePolicy>
std::vector<typename MergePolicy::ResolvedEntry>
    Sync<MergePolicy>::AddAccountTransferRecord(const typename MergePolicy::UnresolvedEntry& entry,
                                                bool all_account_transfers_received) {
  return AddEntry(entry, all_account_transfers_received);
}

template<typename MergePolicy>
void Sync<MergePolicy>::ReplaceNode(const NodeId& old_node, const NodeId& new_node) {
  auto itr(std::begin(MergePolicy::unresolved_data_));
  while (itr != std::end(MergePolicy::unresolved_data_))
    itr = ReplaceNodeAndIncrementItr(itr, old_node, new_node);
}

template<typename MergePolicy>
void Sync<MergePolicy>::ReplaceNode(const typename MergePolicy::DbKey& db_key,
                                    const NodeId& old_node,
                                    const NodeId& new_node) {
  auto itr(std::begin(MergePolicy::unresolved_data_));
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    if (itr->key.first == db_key)
      itr = ReplaceNodeAndIncrementItr(itr, old_node, new_node);
    else
      ++itr;
  }
}

template<typename MergePolicy>
typename MergePolicy::UnresolvedEntriesItr Sync<MergePolicy>::ReplaceNodeAndIncrementItr(
    typename MergePolicy::UnresolvedEntriesItr entries_itr,
    const NodeId& old_node,
    const NodeId& new_node) {
  auto found(detail::FindInMessages<MergePolicy>(*entries_itr, old_node));
  if (found == std::end((*entries_itr).messages_contents)) {
    (*entries_itr).messages_contents.emplace_back();
    (*entries_itr).messages_contents.back().peer_id = new_node;
  } else {
    (*found).peer_id = new_node;
  }

  if (detail::IsResolved<MergePolicy>(*entries_itr)) {
    MergePolicy::Merge(*entries_itr);
    entries_itr = MergePolicy::unresolved_data_.erase(entries_itr);
  } else {
    ++entries_itr;
  }
  return entries_itr;
}

template<typename MergePolicy>
std::vector<typename MergePolicy::UnresolvedEntry> Sync<MergePolicy>::GetUnresolvedData(
    bool increment_sync_attempts) {
  std::vector<typename MergePolicy::UnresolvedEntry> result;
  auto itr = std::begin(MergePolicy::unresolved_data_);
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    ++(*itr).sync_counter;
    if (CanBeErased(*itr)) {
      itr = MergePolicy::unresolved_data_.erase(itr);
      continue;
    }
    auto found(detail::FindInMessages<MergePolicy>(*itr, this_node_id_));
    if (found != std::end(itr->messages_contents)) {
      assert(itr->key.second != nfs::MessageAction::kAccountTransfer);
      // Always move the found message (i.e. this node's message) to the front of the vector.  This
      // serves as an indicator that the entry has not been synchronised by this node to the peers
      // if its message is not the first in the vector.  (It's also slightly more efficient to find
      // in future GetUnresolvedData attempts since we search from begin() to end()).
      result.push_back(*itr);
      result.back().messages_contents.assign(1, *found);
      std::iter_swap(found, std::begin(itr->messages_contents));
    }
    ++itr;
  }
  return result;
}

template<typename MergePolicy>
size_t Sync<MergePolicy>::GetUnresolvedCount(const typename MergePolicy::DbKey& db_key) const {
  return std::count_if(MergePolicy::unresolved_data_.begin(),
                       MergePolicy::unresolved_data_.end(),
                       [&db_key] (const typename MergePolicy::UnresolvedEntry& unresolved_data) {
                           return (unresolved_data.key.first == db_key);
                       });
}

template<typename MergePolicy>
bool Sync<MergePolicy>::CanBeErased(const typename MergePolicy::UnresolvedEntry& entry) const {
  return entry.sync_counter > sync_counter_max_ ||
         detail::IsResolvedOnAllPeers<MergePolicy>(entry, this_node_id_);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
