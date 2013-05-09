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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_

#include <cstdint>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class PmidRecord; }

struct PmidRecord {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidRecordTag> serialised_type;

  PmidRecord();
  explicit PmidRecord(const PmidName& pmid_name_in);
  explicit PmidRecord(const protobuf::PmidRecord& proto_pmid_record);
  protobuf::PmidRecord ToProtobuf() const;
  explicit PmidRecord(const serialised_type& serialised_pmid_record);
  serialised_type Serialise() const;
  PmidRecord(const PmidRecord& other);
  PmidRecord(PmidRecord&& other);
  PmidRecord& operator=(PmidRecord other);

  PmidName pmid_name;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
  int64_t claimed_available_size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_
