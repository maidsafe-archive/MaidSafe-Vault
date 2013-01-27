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

#include <cstdint>
#include <deque>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/vault/pmid_record.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class PutData; }

class MaidAccount {
 public:
  typedef MaidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidAccountTag> serialised_type;
  MaidAccount(const MaidName& maid_name, const boost::filesystem::path& root);
  MaidAccount(const serialised_type& serialised_maid_account, const boost::filesystem::path& root);
  serialised_type Serialise() const;

  void RegisterPmid(const nfs::PmidRegistration::serialised_type& serialised_pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  void UpdatePmidTotals(const PmidTotals& pmid_totals);

  std::vector<boost::filesystem::path> GetArchiveFileNames() const;
  NonEmptyString GetArchiveFile(const boost::filesystem::path& path) const;
  void PutArchiveFile(const boost::filesystem::path& path, const NonEmptyString& content);

  template<typename Data>
  void PutData(const typename Data::name_type& name, int32_t size, int32_t replication_count);
  template<typename Data>
  bool DeleteData(const typename Data::name_type& name);
  template<typename Data>
  void UpdateReplicationCount(const typename Data::name_type& name, int32_t new_replication_count);

  MaidName name() const { return kMaidName_; }
  int64_t total_data_stored_by_pmids() const { return total_data_stored_by_pmids_; }
  int64_t total_put_data() const { return total_put_data_; }

 private:
  struct PutDataDetails {
    PutDataDetails();
    PutDataDetails(const DataNameVariant& data_name_variant_in,
                   int32_t size_in,
                   int32_t replications_in);
    PutDataDetails(const PutDataDetails& other);
    PutDataDetails& operator=(const PutDataDetails& other);
    PutDataDetails(PutDataDetails&& other);
    PutDataDetails& operator=(PutDataDetails&& other);

    DataNameVariant data_name_variant;
    int32_t size, replications;
  };

  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);
  MaidAccount(MaidAccount&&);
  MaidAccount& operator=(MaidAccount&&);
  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  const MaidName kMaidName_;
  GetTagValueAndIdentityVisitor type_and_name_visitor_;
  std::vector<PmidTotals> pmid_totals_;
  std::deque<PutDataDetails> recent_put_data_;
  int64_t total_data_stored_by_pmids_, total_put_data_;
  DiskBasedStorage archive_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_H_
