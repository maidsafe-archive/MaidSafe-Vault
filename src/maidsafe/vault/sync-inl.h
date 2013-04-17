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
#include "maidsafe/vault/db.h"
#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {

template<typename MergePolicy>
Sync<MergePolicy>::Sync(Db* db) : MergePolicy(db) {}

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
Sync<MergePolicy>::FindUnresolved(const DataNameVariant& key_to_find) {
  return std::find_if(std::begin(MergePolicy::unresolved_data_), std::end(MergePolicy::unresolved_data_),
      [] (const std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>> &test) {
        if (std::get<0>(test) == key_to_find)
          return true;
   });
}

template<typename MergePolicy>
void Sync<MergePolicy>::AddMessage(const DataNameVariant& key,
                                   const NonEmptyString& value,
                                   nfs::MessageAction message_action,
                                   const NodeId& node_id) {
  auto found = FindUnresolved(key);  // TODO find all keys as we may have different message_actions
  if (found == std::end(unresolved_data_)) {
    std::set<NodeId> temp;
    temp.insert(node_id);
    unresolved_data_.insert(std::make_tuple(key, value, message_action, temp));
  } else {
    std::get<3>(*found).insert(key);
    if ((std::get<3>(*found).size >= (routing::Parameters::node_group_size + 1) / 2) &&
        (std::get<3>(*found) == message_action)) {
      MergePolicy::Merge(key, value, message_action);
      unresolved_data_.erase(found);
    }
  }
}

// iterate the unresolved data and insert new node key in every element
// replacing the old node keys if found. This allows us to catch up on any gaps in the messages
template<typename MergePolicy>
void Sync<MergePolicy>::ReplaceNode(const NodeId& old_node, const NodeId& new_node) {
  for(const auto& element: unresolved_data_) {
    auto found = std::find(std::begin(std::get<3>(element)), std::end(std::get<3>(element)),
                           old_node);
    if (found != std::end(std::get<3>(element)))
      std::get<3>(element).erase(found);
    std::get<3>(element).insert(new_node);
    if (std::get<3>(element).size >= (routing::Parameters::node_group_size + 1) / 2) {
      CopyToDataBase(std::get<0>(element), std::get<1>(element), std::get<2>(element));
//this won't work - do after loop?      unresolved_data_.erase(found);
    }

  }
}

// TODO this will be the Merge method that each class template must provide.
template<typename MergePolicy>
void Sync<MergePolicy>::CopyToDataBase(const DataNameVariant& key,
                                       const NonEmptyString& value,
                                       nfs::MessageAction message_action) {
  leveldb::WriteOptions write_options;
  write_options.sync = false;  // fast but may lose some data on crash
  leveldb::Slice key = std::get<0>(*found).string();
  leveldb::Slice db_value = value;
  db_->Put(write_options, key, db_value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
