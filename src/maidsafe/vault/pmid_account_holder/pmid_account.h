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

#include <cstdint>
#include <deque>
#include <vector>
#include <utility>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account_merge_policy.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account.pb.h"
#include "maidsafe/vault/pmid_account_holder/pmid_record.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class Db;
class AccountDb;

class PmidAccount {
 public:
  typedef PmidName name_type;
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidAccountTag> serialised_type;

  struct DataElement {
    DataElement();
    DataElement(const DataNameVariant& data_name_variant_in, int32_t size_in);
    DataElement(const DataElement& other);
    DataElement& operator=(const DataElement& other);
    DataElement(DataElement&& other);
    DataElement& operator=(DataElement&& other);
    protobuf::DataElement ToProtobuf() const;

    DataNameVariant data_name_variant;
    int32_t size;
  };
  enum class DataHolderStatus : int32_t { kDown, kGoingDown, kUp, kGoingUp };

  PmidAccount(const PmidName& pmid_name, Db &db, const NodeId &this_node_id);
  PmidAccount(const PmidName& pmid_name,
              Db& db,
              const NodeId& this_node_id,
              const NodeId& source_id,
              const serialised_type& serialised_pmid_account_details);

  void SetDataHolderUp() { data_holder_status_ = DataHolderStatus::kUp; }
  void SetDataHolderDown() { data_holder_status_ = DataHolderStatus::kDown; }


  void PutData(int32_t size) {
    pmid_record_.stored_total_size += size;
  }

  template<typename Data>
  void DeleteData(const typename Data::name_type& name) {
    pmid_record_.stored_count--;
    pmid_record_.stored_total_size -= sync_.AllowDelete<Data>(name);
  }
  serialised_type Serialise();
  name_type name() const { return pmid_name_; }
  DataHolderStatus data_holder_status() const { return data_holder_status_; }
  int64_t total_data_stored_by_pmids() const { return pmid_record_.stored_total_size; }

 private:
  PmidAccount(const PmidAccount&);
  PmidAccount& operator=(const PmidAccount&);
  PmidAccount(PmidAccount&&);
  PmidAccount& operator=(PmidAccount&&);

  name_type pmid_name_;
  NodeId this_node_id_;
  PmidRecord pmid_record_;
  DataHolderStatus data_holder_status_;
  std::unique_ptr<AccountDb> account_db_;
  Sync<PmidAccountMergePolicy> sync_;
};

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_H_
