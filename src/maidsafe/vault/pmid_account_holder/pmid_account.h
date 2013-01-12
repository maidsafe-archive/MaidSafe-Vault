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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_

#include <vector>
#include <mutex>

#include "maidsafe/common/types.h"

#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

struct PmidRecord {
 public:
  explicit PmidRecord(const PmidName& pmid_name);
  explicit PmidRecord(const NonEmptyString& serialised_pmid_record);
  NonEmptyString Serialise() const;

  const PmidName kPmidName;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_
