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

#include "maidsafe/vault/maid_account_holder/maid_account_sync.h"


namespace maidsafe {

namespace vault {

MaidAccountSync::MaidAccountSync(const MaidName& account_name)
    : mutex_(),
      kMaidName_(account_name),
      downloaded_files_(),
      sync_updates_(),
      is_ready_for_merge_(false) {}

}  // namespace vault

}  // namespace maidsafe
