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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_

#include <cstdint>
#include <deque>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/pmid_account_holder/pmid_record.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace test {
class MaidAccountHandlerTest;

template<typename Data>
class MaidAccountHandlerTypedTest;

}  // namespace test


struct PmidTotals {
  PmidTotals();
  PmidTotals(const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in,
             const PmidRecord& pmid_record_in);
  PmidTotals(const PmidTotals& other);
  PmidTotals& operator=(const PmidTotals& other);
  PmidTotals(PmidTotals&& other);
  PmidTotals& operator=(PmidTotals&& other);

  nfs::PmidRegistration::serialised_type serialised_pmid_registration;
  PmidRecord pmid_record;
};

class MaidAccount {
 public:
  typedef MaidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidAccountTag> serialised_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidAccountInfoTag> serialised_info_type;

  struct PutDataDetails {
    PutDataDetails();
    PutDataDetails(const DataNameVariant& data_name_variant_in, int32_t cost_in);
    PutDataDetails(const PutDataDetails& other);
    PutDataDetails& operator=(const PutDataDetails& other);
    PutDataDetails(PutDataDetails&& other);
    PutDataDetails& operator=(PutDataDetails&& other);

    DataNameVariant data_name_variant;
    int32_t cost;
  };

  struct AccountInfo {
    GetTagValueAndIdentityVisitor type_and_name_visitor;
    std::vector<PmidTotals> pmid_totals;
    std::deque<PutDataDetails> recent_put_data;
    int64_t total_claimed_available_size_by_pmids, total_put_data;
  };

  MaidAccount(const MaidName& maid_name, const boost::filesystem::path& root);
  MaidAccount(const serialised_type& serialised_maid_account, const boost::filesystem::path& root);
  serialised_type Serialise() const;

  serialised_info_type  SerialiseAccountInfo() const;
  AccountInfo ParseAccountInfo(const serialised_info_type& serialised_info) const;

  void RegisterPmid(const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  void UpdatePmidTotals(const PmidTotals& pmid_totals);

  std::vector<boost::filesystem::path> GetArchiveFileNames() const;
  NonEmptyString GetArchiveFile(const boost::filesystem::path& filename) const;
  void PutArchiveFile(const boost::filesystem::path& filename, const NonEmptyString& content);

  // This offers the strong exception guarantee
  template<typename Data>
  void PutData(const typename Data::name_type& name, int32_t cost);
  // This offers the strong exception guarantee
  template<typename Data>
  void DeleteData(const typename Data::name_type& name);

  template<typename Data>
  void Adjust(const typename Data::name_type& name, int32_t cost);

  name_type name() const { return kMaidName_; }

  friend class test::MaidAccountHandlerTest;
  template<typename Data>
  friend class test::MaidAccountHandlerTypedTest;

 private:

  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);
  MaidAccount(MaidAccount&&);
  MaidAccount& operator=(MaidAccount&&);
  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  template<typename Data>
  void DoPutData(const typename Data::name_type& name, int32_t cost);

  const name_type kMaidName_;
  GetTagValueAndIdentityVisitor type_and_name_visitor_;
  std::vector<PmidTotals> pmid_totals_;
  std::deque<PutDataDetails> recent_put_data_;
  int64_t total_claimed_available_size_by_pmids_, total_put_data_;
  DiskBasedStorage archive_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder/maid_account-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
