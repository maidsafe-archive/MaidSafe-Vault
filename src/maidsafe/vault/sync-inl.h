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
  for (;;) {
    auto found = std::find_if(std::begin(MergePolicy::unresolved_data_),
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

    if (merge &&
        (*found).messages_contents.size() >= (routing::Parameters::node_group_size + 1U) / 2) {
      MergePolicy::Merge(*found);
      MergePolicy::unresolved_data_.erase(found);
      return true;
    }
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
    auto found(std::find_if(
        std::begin((*itr).messages_contents),
        std::end((*itr).messages_contents),
        [&old_node](const typename MergePolicy::UnresolvedEntry::MessageContent& content) {
            return content.peer_id == old_node;
        }));
    if (found == std::end((*itr).messages_contents)) {
      (*itr).messages_contents.emplace_back();
      (*itr).messages_contents.back().peer_id = new_node;
    } else {
      (*found).peer_id = new_node;
    }

    if ((*itr).messages_contents.size() >=
        static_cast<size_t>((routing::Parameters::node_group_size + 1) / 2)) {
      MergePolicy::Merge(*itr);
      itr = MergePolicy::unresolved_data_.erase(itr);
    } else {
      ++itr;
    }
  }
}

template<typename MergePolicy>
std::vector<typename MergePolicy::UnresolvedEntry> Sync<MergePolicy>::GetUnresolvedData() const {
  std::vector<typename MergePolicy::UnresolvedEntry> return_vec;
  for (const auto& entry : MergePolicy::unresolved_data_) {
    auto found(std::find_if(
        std::begin(entry.messages_contents),
        std::end(entry.messages_contents),
        [this](const typename MergePolicy::UnresolvedEntry::MessageContent& content) {
            return content.peer_id == this->this_node_id_;
        }));
    if (found != std::end(entry.messages_contents)) {
      return_vec.push_back(entry);
      return_vec.back().messages_contents.assign(1, *found);
    }
  }
  return return_vec;
}

template<typename MergePolicy>
void Sync<MergePolicy>::IncrementSyncAttempts() {
  auto itr = std::begin(MergePolicy::unresolved_data_);
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    if (++(*itr).sync_counter > sync_counter_max_) {
      itr = MergePolicy::unresolved_data_.erase(itr);
    } else {
      ++itr;
    }
  }
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
