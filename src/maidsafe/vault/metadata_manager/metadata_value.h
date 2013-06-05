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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_VALUE_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_VALUE_H_

#include <cstdint>
#include <vector>

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/metadata_manager/metadata.pb.h"
#include "maidsafe/vault/metadata_manager/metadata_manager.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

struct MetadataValue {
  typedef TaggedValue<NonEmptyString, struct SerialisedMetadataValueTag> serialised_type;
  explicit MetadataValue(const serialised_type& serialised_metadata_value);
  explicit MetadataValue(int size_in);
  serialised_type Serialise() const;

  int data_size;
  boost::optional<int64_t> subscribers;
  std::set<PmidName> online_pmid_name, offline_pmid_name;
};

class Metadata {
 public:
  // This constructor reads the existing element or creates a new one if it doesn't already exist.
  Metadata(const DataNameVariant& data_name,
           ManagerDb<MetadataManager>* metadata_db,
           int32_t data_size);
  // This constructor reads the existing element or throws if it doesn't already exist.
  Metadata(const DataNameVariant& data_name, ManagerDb<MetadataManager>* metadata_db);
  // Should only be called once.
  void SaveChanges(ManagerDb<MetadataManager>* metadata_db);

  DataNameVariant data_name_;
  MetadataValue value_;
  on_scope_exit strong_guarantee_;

 private:
  Metadata();
  Metadata(const Metadata&);
  Metadata& operator=(const Metadata&);
  Metadata(Metadata&&);
  Metadata& operator=(Metadata&&);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_VALUE_H_
