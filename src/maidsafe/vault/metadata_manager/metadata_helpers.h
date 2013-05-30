/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HELPERS_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HELPERS_H_

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

struct MetadataValueDelta {
  int data_size;
  boost::optional<int64_t> subscribers;
  std::vector<PmidName> new_online; // FIXME(Prakash) discuss
  std::vector<PmidName> new_offline;  // FIXME(Prakash) discuss
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HELPERS_H_
