/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_SYNC_H_
#define MAIDSAFE_VAULT_SYNC_H_

#include <cstdint>
#include <vector>
#include <tuple>
#include <set>
#include <memory>
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include "leveldb/db.h"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"

namespace maidsafe {

namespace vault {

class Sync {
public:
  Sync(boost::filesystem::path account_root_directory);
  void AddMessage(DataNameVariant key, NodeId node_id, std::string value);
  std::map<DataNameVariant, std::string> GetMessages();
  void ReplaceNode(NodeId old_node, NodeId new_node);
  NonEmptyString GetAccountTransfer();
  void ApplyAccountTransfer(NonEmptyString account);
private:
  std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>> unresolved_data_;
  leveldb::DB* db_;
  leveldb::Status db_status_;
  std::vector<std::tuple<DataNameVariant, std::string, std::set<NodeId>>>::iterator
                                                   FindUnresolved(const DataNameVariant&);
  void CopyToDataBase(DataNameVariant key, std::string value);


};


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SYNC_H_
