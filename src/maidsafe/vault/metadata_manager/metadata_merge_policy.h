/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_element.h"


namespace maidsafe {

namespace vault {

class MetadataMergePolicy {
 public:
  typedef MetadataUnresolvedEntry UnresolvedEntry;
  explicit MetadataMergePolicy(const boost::filesystem::path& root);
  MetadataMergePolicy(MetadataMergePolicy&& other);
  MetadataMergePolicy& operator=(MetadataMergePolicy&& other);
  // This flags a "Put" entry in 'unresolved_data_' as not to be added to the db.
  template<typename Data>
  int32_t AllowDelete(const typename Data::name_type& name);

 protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  boost::filesystem::path metadata_root_;
  std::vector<UnresolvedEntry> unresolved_data_;

 private:
  MetadataMergePolicy(const MetadataMergePolicy&);
  MetadataMergePolicy& operator=(const MetadataMergePolicy&);

  void MergePut(const DataNameVariant& data_name,
                UnresolvedEntry::Value cost,
                const NonEmptyString& serialised_db_value);
  void MergeDelete(const DataNameVariant& data_name, const NonEmptyString& serialised_db_value);
//  NonEmptyString SerialiseDbValue(DbValue db_value) const;
//  DbValue ParseDbValue(NonEmptyString serialised_db_value) const;
//  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MERGE_POLICY_H_
