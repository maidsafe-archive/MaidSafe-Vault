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

#include <map>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/vault/pmid_record.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class PmidAccount {
 public:
  typedef PmidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidAccountTag> serialised_type;

  enum class Status : int {
    kNodeGoingUp,
    kNodeUp,
    kNodeGoingDown,
    kNodeDown
  };

  PmidAccount(const PmidName& pmid_name, const boost::filesystem::path& root);
  PmidAccount(const serialised_type& serialised_pmid_account,
              const boost::filesystem::path& root);
  ~PmidAccount();

  std::vector<boost::filesystem::path> GetArchiveFileNames() const;
  NonEmptyString GetArchiveFile(const boost::filesystem::path& path) const;
  void PutArchiveFile(const boost::filesystem::path& path, const NonEmptyString& content);
  void ArchiveRecords();
  serialised_type Serialise() const;

  template<typename Data>
  void PutData(const typename Data::name_type& name, int32_t size, int32_t replication_count);
  template<typename Data>
  bool DeleteData(const typename Data::name_type& name);

  PmidName name() const { return pmid_record_.pmid_name; }
  Status GetStatus() const { return account_status_; }
  void SetStatus(Status status) { account_status_ = status; }
  int64_t total_data_stored_by_pmids() const { return pmid_record_.stored_total_size; }
//  int64_t total_put_data() const { return total_put_data_; }
 private:
  PmidAccount(const PmidAccount&);
  PmidAccount& operator=(const PmidAccount&);
  PmidAccount(PmidAccount&&);
  PmidAccount& operator=(PmidAccount&&);

  std::future<void> ArchiveDataRecord(const DataNameVariant& data_name_variant,
                                      const int32_t data_size);

  Status account_status_;
  PmidRecord pmid_record_;
  std::map<DataNameVariant, int32_t> recent_data_stored_;
  GetTagValueAndIdentityVisitor type_and_name_visitor_;
  DiskBasedStorage archive_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_account-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_H_
