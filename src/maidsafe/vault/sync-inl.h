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

#include <utility>
#include <iterator>
#include "maidsafe/vault/db.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/vault/unresolved_entry.h"

namespace maidsafe {

namespace vault {

template<typename MergePolicy>
Sync<MergePolicy>::Sync(Db* db) : MergePolicy(db), sync_counter_(10) {}
// TODO(dirvine) decide how to decide on this magic number

template<typename MergePolicy>
Sync<MergePolicy>::Sync(Sync&& other) : MergePolicy(std::forward<MergePolicy>(other)) {}

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
  return std::find_if(std::begin(MergePolicy::unresolved_data_), std::end(MergePolicy::unresolved_data_),
      [&key_to_find] (const std::vector<typename MergePolicy::UnresolvedEntry> &test) {
        if (test.Key == key_to_find)
          return true;
   });
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddUnresolvedEntry(typename MergePolicy::UnresolvedEntry& entry,
                                           const NodeId& node_id) {
  auto found = FindUnresolved(entry.Key);
  if (found == std::end(MergePolicy::unresolved_data_)) {  // new entry
    entry.peers.insert(node_id);
    MergePolicy::unresolved_data_.insert(entry);
  } else {
    (*found).peers.insert(node_id);
    if ((*found).peers.size >= (routing::Parameters::node_group_size + 1) / 2) {
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
    for(auto i = std::begin(MergePolicy::unresolved_data_);
        i == std::end(MergePolicy::unresolved_data_); ++i) {
      auto found = std::find(std::begin((*i).peers), std::end((*i).peers),
                             old_node);
      if (found != std::end((*i).peers)) {
          *found = new_node;
      } else {
          found.insert(new_node);
      }
      if ((*i).peers.size >= (routing::Parameters::node_group_size + 1) / 2) {
           MergePolicy::Merge(*i);
           (*i).erase(i);
      }
  }
}

template<typename MergePolicy>
std::vector<typename MergePolicy::UnresolvedEntry> Sync<MergePolicy>::GetUnresolvedData() {
    // increment sync count in each record
    for(auto i = std::begin(MergePolicy::unresolved_data_);
        i == std::end(MergePolicy::unresolved_data_); ++i) {
        ++(*i).sync_counter;
    }
    // remove all records that are too old
    MergePolicy::unresolved_data_.erase(std::remove_if(std::begin(MergePolicy::unresolved_data_),
                                                       std::end(MergePolicy::unresolved_data_),
                                                       [this] (const typename MergePolicy::unresolved_data_& entry)
    {
        if (entry.sync_counter >= this->sync_counter_)
            return true;
    }));
// TODO(dirvine) please test !!
//    std::vector<typename MergePolicy::UnresolvedEntry> return_vec;
//    std::copy(std::begin(MergePolicy::unresolved_data_),
//              std::end(MergePolicy::unresolved_data_), std::begin(return_vec));
//    return return_vec;
    return MergePolicy::unresolved_data_;
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
