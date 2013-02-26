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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_HANDLER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_HANDLER_H_

#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/maid_account_holder/maid_account_sync.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class MaidAccountSyncHandler {
 public:
  explicit MaidAccountSyncHandler(const boost::filesystem::path& vault_root_dir);

 private:
  MaidAccountSyncHandler(const MaidAccountSyncHandler&);
  MaidAccountSyncHandler& operator=(const MaidAccountSyncHandler&);
  MaidAccountSyncHandler(MaidAccountSyncHandler&&);
  MaidAccountSyncHandler& operator=(MaidAccountSyncHandler&&);

  const boost::filesystem::path kMaidAccountsSyncRoot_;
  mutable std::mutex mutex_;

  std::vector<MaidAccountSync> maid_accounts_sync_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_HANDLER_H_
