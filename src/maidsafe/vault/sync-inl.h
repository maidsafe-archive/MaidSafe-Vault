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
// TODO(dirvine) possibly alter logic here to check if existing_entry and new_entry can be used
// and abstract away the fact that entry_id is optional (check internally for such a thing)
template<typename MergePolicy>
bool PeerAndIdInUnresolved(const typename MergePolicy::UnresolvedEntry& entry,
                      const NodeId& peer_id_to_find,
                      const int32_t entry_id_to_find) {
  return std::any_of(
        std::begin(entry.messages_contents),
        std::end(entry.messages_contents),
        [&](const typename MergePolicy::UnresolvedEntry &test) {
            return test.peer_id == peer_id_to_find &&
                   test.entry_id == entry_id_to_find;
        });
}

template<typename MergePolicy>
bool PeerInUnresolved(const typename MergePolicy::UnresolvedEntry& entry,
                      const NodeId& peer_id_to_find) {
  return std::any_of(
        std::begin(entry.messages_contents),
        std::end(entry.messages_contents),
        [&](const typename MergePolicy::UnresolvedEntry &test) {
            return test.peer_id == peer_id_to_find;
        });
}

template<typename MergePolicy>
bool EntryIsUnique(const typename MergePolicy::UnresolvedEntry& entry,
                   const NodeId& peer_id_to_find,
                   const int32_t entry_id_to_find) {
  return !PeerAndIdInUnresolved(entry, peer_id_to_find, entry_id_to_find) ||
         PeerInUnresolved(entry, peer_id_to_find);
}

template<typename MergePolicy>
bool EntryIsUnique(const typename MergePolicy::UnresolvedEntry& entry,
                   const NodeId& peer_id_to_find) {
  return !PeerInUnresolved(entry, peer_id_to_find);
}


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
typename std::vector<typename MergePolicy::UnresolvedEntry>::iterator
Sync<MergePolicy>::FindUnresolved(
    typename std::vector<typename MergePolicy::UnresolvedEntry>::iterator begin,
    typename std::vector<typename MergePolicy::UnresolvedEntry>::iterator end,
    const typename MergePolicy::UnresolvedEntry::Key& key_to_find) {
  return std::find_if(
      begin,
      end,
      [&key_to_find](const typename MergePolicy::UnresolvedEntry &test) {
          return test.key == key_to_find;
      });
}


template<typename MergePolicy>
void Sync<MergePolicy>::AddLocalEntry(typename MergePolicy::UnresolvedEntry& entry) {

}

template<typename MergePolicy>
bool Sync<MergePolicy>::AddUnresolvedEntry(typename MergePolicy::UnresolvedEntry& entry) {
  auto end = std::end(MergePolicy::unresolved_data_);
  auto begin = std::begin(MergePolicy::unresolved_data_);
  auto found = FindUnresolved(begin, end, entry.key);

  if (found == end) {  // new entry
    MergePolicy::unresolved_data_.push_back(entry);
    return true;   // TODO(dirvine) why ?
  }
  // TODO (dirvine) FINISH ME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  while (found != end) {
    if (!PeerAndIdInEntry((*found),
                          entry.messages_content.first().peer_id,
                          *entry.messages_content.first().peer_id) {
      (*found).messages_content.insert(messages_content(node_id));
      if ((*found).peers.size() >= (routing::Parameters::node_group_size + 1U) / 2) {
        entry.peers.clear();
        MergePolicy::Merge(entry);
        MergePolicy::unresolved_data_.erase(found);
        return true;
     }
   }
  }
  return false;
}

template<typename MergePolicy>
bool Sync<MergePolicy>::AddAccountTransferRecord(typename MergePolicy::UnresolvedEntry& entry,
                                                 const NodeId& node_id,
                                                 bool all_account_transfers_received) {
  auto found = FindUnresolved(entry.key());
  if (found == std::end(MergePolicy::unresolved_data_)) {  // new entry
    entry.peers.insert(node_id);
    MergePolicy::unresolved_data_.push_back(entry);
  } else {
    if (all_account_transfers_received) {
      entry.peers.clear();
      MergePolicy::Merge(entry);
      MergePolicy::unresolved_data_.erase(found);
      return true;
    } else {
      (*found).peers.insert(node_id);
    }
  }
  return false;
}

// iterate the unresolved data and insert new node key in every element
// replacing the old node keys if found. This is to absolve us from altering the close
// node size for any messages that are partially resolved
template<typename MergePolicy>
void Sync<MergePolicy>::ReplaceNode(const NodeId& old_node, const NodeId& new_node) {
  for (auto itr = std::begin(MergePolicy::unresolved_data_);
       itr != std::end(MergePolicy::unresolved_data_); ++itr) {
    (*itr).peers.insert(new_node);
    auto found = std::find(std::begin((*itr).peers), std::end((*itr).peers), old_node);
    if (found != std::end((*itr).peers))
        (*itr).peers.erase(found);

    if ((*itr).peers.size() >=
        static_cast<size_t>((routing::Parameters::node_group_size + 1) / 2)) {
      MergePolicy::Merge(*itr);
      MergePolicy::unresolved_data_.erase(itr);
    }
  }
}

template<typename MergePolicy>
std::vector<typename MergePolicy::UnresolvedEntry> Sync<MergePolicy>::GetUnresolvedData() {
  // increment sync count in each record or remove records if it's too old
  auto itr = std::begin(MergePolicy::unresolved_data_);
  while (itr != std::end(MergePolicy::unresolved_data_)) {
    if (++(*itr).sync_counter > sync_counter_max_) {
      itr = MergePolicy::unresolved_data_.erase(itr);
    } else {
      ++itr;
    }
  }

  // only supply data containing this node's ID
  std::vector<typename MergePolicy::UnresolvedEntry> return_vec;
  std::copy_if(std::begin(MergePolicy::unresolved_data_),
               std::end(MergePolicy::unresolved_data_),
               std::begin(return_vec), [this](const typename MergePolicy::UnresolvedEntry& entry) {
                   return (entry.peers.find(this->this_node_id_) != std::end(entry.peers));
               });
  return return_vec;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
