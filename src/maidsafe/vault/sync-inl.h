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

#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/account_db.h"


namespace maidsafe {

namespace vault {

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
Sync<MergePolicy>::FindUnresolved(const typename MergePolicy::UnresolvedEntry::Key& key_to_find) {
  return std::find_if(
      std::begin(MergePolicy::unresolved_data_),
      std::end(MergePolicy::unresolved_data_),
      [&key_to_find](const typename MergePolicy::UnresolvedEntry &test) {
          return test.key() == key_to_find;
      });
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddUnresolvedEntry(typename MergePolicy::UnresolvedEntry& entry,
                                           const NodeId& node_id) {
  auto found = FindUnresolved(entry.key());
  if (found == std::end(MergePolicy::unresolved_data_)) {  // new entry
    entry.peers.insert(node_id);
    MergePolicy::unresolved_data_.push_back(entry);
  } else {
    (*found).peers.insert(node_id);
    if ((*found).peers.size() >= (routing::Parameters::node_group_size + 1) / 2) {
      entry.peers.clear();
      MergePolicy::Merge(entry);
      MergePolicy::unresolved_data_.erase(found);
    }
  }
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

    if ((*itr).peers.size() >= (routing::Parameters::node_group_size + 1) / 2) {
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
