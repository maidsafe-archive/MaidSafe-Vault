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

class PmidSize {
 public:
  explicit PmidSize(Identity pmid_id_in)
    : pmid_id(pmid_id_in),
      num_data_elements(0),
      total_size(0),
      lost_size(0),
      lost_number_of_elements(0) {}
  explicit PmidSize(const NonEmptyString& serialised_pmidsize)
    : pmid_id(),
      num_data_elements(0),
      total_size(0),
      lost_size(0),
      lost_number_of_elements(0) {
    Parse(serialised_pmidsize);
  }

  void Parse(const NonEmptyString& serialised_pmidsize);
  NonEmptyString Serialise();

 private:
  Identity pmid_id;
  int32_t num_data_elements;
  int64_t total_size;
  int64_t lost_size;
  int64_t lost_number_of_elements;
};

class PmidTotal {
 public:
  PmidTotal(nfs::PmidRegistration registration_in, PmidSize pmid_size_in)
    : registration(registration_in), pmid_size(pmid_size_in) {}

  NonEmptyString Serialise();
  bool IsRecordOf(Identity& pmid_id) const {
    return pmid_id == registration.pmid_id();
  }
  Identity pmid_id() { return registration.pmid_id(); }

 private:
  nfs::PmidRegistration registration;
  PmidSize pmid_size;
};

class DataElement {
 public:
  DataElement() : data_id_(), data_size(0) {}

  DataElement(Identity data_id_in, int32_t data_size_in)
    : data_id_(data_id_in), data_size(data_size_in) {}

  NonEmptyString Serialise();
  Identity data_id() const { return data_id_; }

 private:
  Identity data_id_;
  int32_t data_size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_
