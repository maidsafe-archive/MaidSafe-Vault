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

#include "maidsafe/vault/maid_account_holder.h"

namespace maidsafe {

namespace vault {

MaidAccountHolder::MaidAccountHolder(routing::Routing& routing,
                                     const boost::filesystem::path& vault_root_dir)
  : kRootDir_(vault_root_dir / "maids"),
    nfs_(routing, pmid) {}

MaidAccountHolder::~MaidAccountHolder() {
}

}  // namespace vault

}  // namespace maidsafe
