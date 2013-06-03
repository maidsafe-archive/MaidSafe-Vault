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
#include "maidsafe/vault/manager_db.h"

#include <utility>


#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {
namespace vault {

template<>
std::vector<StructuredDataManager::DbKey> ManagerDb<StructuredDataManager>::GetKeys() {
  std::vector<StructuredDataManager::DbKey> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
    std::string name(iter->key().ToString().substr(0, NodeId::kSize));
    std::string type_string(iter->key().ToString().substr(NodeId::kSize + 2));
    Identity identity(iter->key().ToString().substr(NodeId::kSize + 2 ,
                                                    (NodeId::kSize * 2) + 2));
    DataTagValue type = static_cast<DataTagValue>(std::stoul(type_string));
    auto key = std::make_pair(GetDataNameVariant(type, Identity(name)), identity);
    return_vector.push_back(std::move(key));
  }
  return return_vector;
}

}  // namespace vault
}  // namespace maidsafe
