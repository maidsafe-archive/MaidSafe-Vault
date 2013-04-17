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

//template<typename MergePolicy>
//Sync<MergePolicy>::Sync<MergePolicy>(Db* db_wrapper) : MergePolicy(db_wrapper) {}

template<typename MergePolicy>
Sync<MergePolicy>::Sync(Sync&& other) : MergePolicy(std::forward<MergePolicy>(other)) {}

template<typename MergePolicy>
class Sync<MergePolicy>& Sync<MergePolicy>::operator=(Sync&& other) {
  // TODO(Fraser#5#): 2013-04-15 - Remove this once MSVC implements default move assignment.
  using std::swap;
  Sync<MergePolicy> temp(std::move(other));
  swap(*this, temp);
  return *this;
}

template<typename MergePolicy>
std::vector<std::tuple<DataNameVariant, NonEmptyString, std::set<NodeId>>>::iterator
Sync::FindUnresolved<MergePolicy>(const DataNameVariant& key_to_find) {
  return std::find_if(std::begin(unresolved_data_), std::end(unresolved_data_),
      [] (const std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>> &test) {
        if (std::get<0>(test) == key_to_find)
          return true;
   });
}

template<typename MergePolicy>
void Sync::AddMessage<MergePolicy>(const DataNameVariant& key,
                                   const NonEmptyString& value,
                                   const NodeId& node_id,
                                   const nfs::MessageType& message_type) {
  auto found = FindUnresolved(key);  // TODO find all keys as we may have different message_types
  if (found == std::end(unresolved_data_)) {
    std::set<NodeId> temp;
    temp.insert(node_id);
    unresolved_data_.insert(std::make_tuple(key, value, temp, message_type));
  } else {
    std::get<2>(*found).insert(key);
    if ((std::get<2>(*found).size >= (routing::Parameters::node_group_size + 1) / 2 ) &&
        std::get<2>(*found) == message_type) {
      CopyToDataBase(key, value, message_type);
      unresolved_data_.erase(found);
    }
  }
}

// iterate the unresolved data and insert new node key in every element
// replacing the old node keys if found. This allows us to catch up on any gaps in the messages
void Sync::ReplaceNode(NodeId old_node, NodeId new_node) {
  for(const auto& element: unresolved_data_) {
    auto found = std::find(std::begin(std::get<2>(element)), std::end(std::get<2>(element)), old_node);
    if (found != std::end(std::get<2>(element))
        std::get<2>(element).erase(found);
    std::get<2>(element).insert(new_node);
    if (std::get<2>(element).size >= (routing::Parameters::node_group_size + 1) / 2 ) {
      CopyToDataBase(std::get<0>(element), std::get<1>(element));
      unresolved_data_.erase(found);
    }

  }
}

// TODO this will be the Merge method that each class template must provide.
void Sync::CopyToDataBase(const DataNameVariant& key,
                          const std::string& value.
                          const nfs::MessageType message_type) {
  leveldb::WriteOptions write_options;
  write_options.sync = false;  // fast but may lose some data on crash
  leveldb::Slice key = std::get<0>(*found).string();
  leveldb::Slice db_value = value;
  db_->Put(write_options, key, db_value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_INL_H_
