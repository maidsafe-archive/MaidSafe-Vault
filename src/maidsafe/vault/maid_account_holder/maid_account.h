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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_H_

#include <vector>
#include <mutex>

#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"


namespace maidsafe {

namespace vault {

class MaidAccount {
 public:
  MaidAccount() : maid_name_(), pmid_totals_(), data_elements_(), mutex_() {}

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

  passport::PublicMaid::name_type maid_name() const { return maid_name_; }

 private:
  passport::PublicMaid::name_type maid_name_;
  std::vector<PmidTotal> pmid_totals_;
  std::vector<DataElement> data_elements_;
  std::mutex mutex_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_H_
