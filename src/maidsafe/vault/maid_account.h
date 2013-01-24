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
#include <map>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/vault/pmid_record.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

struct PmidTotals;

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
  void PutData(const typename Data::name_type& name, int32_t size, int32_t replication_count); -- throw if not enough space
  template<typename Data>
  bool DeleteData(const typename Data::name_type& name); -- throw if data entry doesn't exist
  template<typename Data>
  void UpdateReplicationCount(const typename Data::name_type& name, int32_t new_replication_count);

  MaidName name() const { return kMaidName_; }
  int64_t total_data_stored_by_pmids() const { return total_data_stored_by_pmids_; }
  int64_t total_put_data() const { return total_put_data_; }

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
  typedef std::map<DataNameVariant, PutDataDetails> RecentPutData;

  MaidAccount(const MaidAccount&);
  MaidAccount& operator=(const MaidAccount&);
  MaidAccount(MaidAccount&&);
  MaidAccount& operator=(MaidAccount&&);

  const MaidName kMaidName_;
  std::vector<PmidTotals> pmid_totals_;
  RecentPutData recent_put_data_;
  int64_t total_data_stored_by_pmids_, total_put_data_;
  DiskBasedStorage archive_;
};

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

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_H_
