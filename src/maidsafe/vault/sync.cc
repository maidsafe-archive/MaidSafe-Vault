/***************************************************************************************************
 *  Copyright 2012 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include "maidsafe/vault/sync.h"

#include "maidsafe/routing/parameters.h"

namespace maidsafe {

namespace vault {


Sync::Sync(boost::filesystem::path account_root_directory) :
  unresolved_data_(), db_(), db_status_() {
  leveldb::Options db_options;
  db_options.create_if_missing = true;
  db_status_ = leveldb::DB::Open(db_options, account_root_directory.string(), &db_);
}

std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>>::iterator
Sync::FindUnresolved(const DataNameVariant& key_to_find) {
  return std::find_if(std::begin(unresolved_data_), std::end(unresolved_data_),
      [] (const std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>> &test) {
        if  (std::get<0>(test) == key_to_find)
          return true;
   });
}

void Sync::AddMessage(DataNameVariant key, NodeId node_id, std::string value) {
  auto found = FindUnresolved(key);
  if (found == std::end(unresolved_data_)) {
    std::set<NodeId> temp = {node_id};  //TODO(team) check MSVC supports this
    // temp.insert(node_id);  // otherwise use this
    unresolved_data_.insert(std::make_tuple(key, value, temp));
  } else {
    std::get<2>(*found).insert(key);
    if (std::get<2>(*found).size >= (routing::Parameters::node_group_size + 1) / 2 ) {
      CopyToDataBase(key, value);
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

void Sync::CopyToDataBase(DataNameVariant key, std::string value) {
  leveldb::WriteOptions write_options;
  write_options.sync = false;  // fast but may lose some data on crash
  leveldb::Slice key = std::get<0>(*found).string();
  leveldb::Slice db_value = value;
  db_->Put(write_options, key, db_value);
}

}  // namespace vault

}  // namespace maidsafe
