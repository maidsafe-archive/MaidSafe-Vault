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

#ifndef MAIDSAFE_VAULT_PMID_RECORD_H_
#define MAIDSAFE_VAULT_PMID_RECORD_H_

#include "maidsafe/common/types.h"

#include "maidsafe/vault/pmid_account_pb.h"


namespace maidsafe {

namespace vault {

struct PmidRecord {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidRecordTag> serialised_type;
  explicit PmidRecord(const PmidName& pmid_name);
  explicit PmidRecord(const serialised_type& serialised_pmid_record);
  serialised_type Serialise() const;
  PmidAccount(const PmidAccount&);
  PmidAccount& operator=(const PmidAccount&);
  PmidAccount(PmidAccount&&);
  PmidAccount& operator=(PmidAccount&&);

  const PmidName kPmidName;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_RECORD_H_
