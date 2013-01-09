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

#include "maidsafe/vault/metadata_manager.h"

namespace maidsafe {

namespace vault {

MetaDataManager::MetaDataManager(routing::Routing& routing,
                                 const boost::filesystem::path& /*vault_root_dir*/): routing_(routing) {
}

void MetaDataManager::OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
}

}  // namespace vault

}  // namespace maidsafe
