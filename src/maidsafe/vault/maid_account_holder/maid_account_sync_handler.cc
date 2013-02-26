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

#include "maidsafe/vault/maid_account_holder/maid_account_sync_handler.h"

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

MaidAccountSyncHandler::MaidAccountSyncHandler(const boost::filesystem::path& vault_root_dir)
    : kMaidAccountsSyncRoot_(vault_root_dir / "maids_sync"),
      mutex_(),
      maid_accounts_sync_() {
  fs::exists(kMaidAccountsSyncRoot_) || fs::create_directory(kMaidAccountsSyncRoot_);
}


}  // namespace vault

}  // namespace maidsafe
