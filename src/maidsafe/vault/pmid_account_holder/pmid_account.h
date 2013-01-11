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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_H_

#include <vector>
#include <mutex>

#include "maidsafe/common/types.h"


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
  PmidTotal(PmidRegistration registration_in, PmidSize pmid_size_in)
    : registration(registration_in), pmid_size(pmid_size_in) {}

  NonEmptyString Serialise();
  bool IsRecordOf(Identity& pmid_id) const {
    return pmid_id == registration.pmid_id();
  }
  Identity pmid_id() { return registration.pmid_id(); }

 private:
  PmidRegistration registration;
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

class MaidAccount {
 public:
  MaidAccount() : maid_id_(), pmid_totals_(), data_elements_(), mutex_() {}

  explicit MaidAccount(Identity maid_id_in)
    : maid_id_(maid_id_in), pmid_totals_(), data_elements_(), mutex_() {}

  explicit MaidAccount(const NonEmptyString& serialised_maidaccount)
    : maid_id_(), pmid_totals_(), data_elements_(), mutex_() {
    Parse(serialised_maidaccount);
  }

  MaidAccount(const MaidAccount& other)
    : maid_id_(other.maid_id_),
      pmid_totals_(other.pmid_totals_),
      data_elements_(other.data_elements_),
      mutex_() {}

  MaidAccount& operator=(const MaidAccount& other);

  void Parse(const NonEmptyString& serialised_maidaccount);
  NonEmptyString Serialise();

  void PushPmidTotal(PmidTotal pmid_total);
  void RemovePmidTotal(Identity pmid_id);
  void UpdatePmidTotal(PmidTotal pmid_total);
  bool HasPmidTotal(Identity pmid_id);

  void PushDataElement(DataElement data_element);
  void RemoveDataElement(Identity data_id);
  void UpdateDataElement(DataElement data_element);
  bool HasDataElement(Identity data_id);

  Identity maid_id() const { return maid_id_; }

 private:
  Identity maid_id_;
  std::vector<PmidTotal> pmid_totals_;
  std::vector<DataElement> data_elements_;
  std::mutex mutex_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_H_
