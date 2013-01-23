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

#include <map>
#include <vector>

#include "maidsafe/common/types.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

struct PmidTotals;

class MaidAccount {
 public:
  typedef MaidName name_type;
  explicit MaidAccount(const MaidName& maid_name);
  explicit MaidAccount(const NonEmptyString& serialised_maid_account);
  NonEmptyString Serialise() const;
  void Merge(MaidAccount&& other);

  void RegisterPmid(const NonEmptyString& serialised_pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  void UpdatePmidTotals(const PmidTotals& pmid_totals);

  template<typename Data>
  void PutData(const typename Data::name_type& name, int32_t size, int32_t replications);
  template<typename Data>
  bool DeleteData(const typename Data::name_type& name);
  template<typename Data>
  void UpdateReplications(const typename Data::name_type& name, int32_t replications);

  void PutArchivedData(const boost::filesystem::path& path, const NonEmptyString& archived_data);

  MaidName name() const { return kMaidName_; }

 private:
  struct PutDataDetails {
    PutDataDetails();
    PutDataDetails(int32_t size_in, int32_t replications_in);
    PutDataDetails(const PutDataDetails& other);
    PutDataDetails& operator=(const PutDataDetails& other);
    PutDataDetails(PutDataDetails&& other);
    PutDataDetails& operator=(PutDataDetails&& other);
    int32_t size, replications;
  };
  const MaidName kMaidName_;
  std::vector<PmidTotals> pmid_totals_;
  std::map<DataNameVariant, PutDataDetails> recent_put_data_;
  int64_t total_data_stored_by_pmids_, total_put_data_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_H_
