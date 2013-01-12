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


namespace maidsafe {

namespace vault {

class PmidRecord {
 public:
  explicit PmidRecord(Identity pmid_name_in)
    : pmid_name(pmid_name_in),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {}
  explicit PmidRecord(const NonEmptyString& serialised_pmidsize)
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0) {
    Parse(serialised_pmidsize);
  }

  void Parse(const NonEmptyString& serialised_pmidsize);
  NonEmptyString Serialise();

 private:
  Identity pmid_name;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
};

class PmidTotal {
 public:
  PmidTotal(nfs::PmidRegistration registration_in, PmidRecord pmid_record_in)
    : registration(registration_in), pmid_record(pmid_record_in) {}

  NonEmptyString Serialise();
  bool IsRecordOf(Identity& pmid_id) const {
    return pmid_id == registration.pmid_id();
  }
  Identity pmid_id() { return registration.pmid_id(); }

 private:
  nfs::PmidRegistration registration;
  PmidRecord pmid_record;
};

class DataElement {
 public:
  DataElement() : name_(), size(0) {}

  DataElement(Identity data_name_in, int32_t data_size_in)
    : name_(data_name_in), size(data_size_in) {}

  NonEmptyString Serialise();
  Identity name() const { return name_; }

 private:
  Identity name_;
  int32_t size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_
