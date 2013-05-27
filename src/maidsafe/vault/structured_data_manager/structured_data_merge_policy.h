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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/structured_data_manager/structured_data_key.h"
#include "maidsafe/vault/structured_data_manager/structured_data_value.h"
#include "maidsafe/vault/structured_data_manager/structured_data_db.h"
#include "maidsafe/vault/unresolved_element.h"


namespace maidsafe {

namespace vault {

class StructuredDataMergePolicy {
 public:
  typedef UnresolvedElement<StructuredDataKey, StructuredDataValue> UnresolvedEntry;
  typedef std::pair<DataNameVariant, nfs::PersonaId> DbKey;

  explicit StructuredDataMergePolicy(maidsafe::vault::StructuredDataDb* db);
  StructuredDataMergePolicy(StructuredDataMergePolicy&& other);
  StructuredDataMergePolicy& operator=(StructuredDataMergePolicy&& other);
  typedef TaggedValue<NonEmptyString, struct DatabaseKey> SerialisedKey;
  typedef TaggedValue<NonEmptyString, struct DatabaseValue> SerialisedValue;

protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  StructuredDataDb* db_;

 private:

  StructuredDataMergePolicy(const StructuredDataMergePolicy&);
  StructuredDataMergePolicy& operator=(const StructuredDataMergePolicy&);

  void MergePut(const StructuredDataKey& key, const Identity& new_value, const Identity& old_value);

  void MergeDeleteBranchUntilFork(const StructuredDataKey& key, const Identity& tot);
  void MergeDelete(const StructuredDataKey& key);

  std::vector<Identity> MergeGet(const StructuredDataKey& key);
  void MergeGetBranch(const StructuredDataKey& key, const Identity& tot);


  SerialisedValue SerialiseDbValue(const StructuredDataValue& db_value) const;
  SerialisedKey SerialiseDbKey(const StructuredDataKey& db_key) const;
  StructuredDataValue ParseDbValue(const SerialisedValue& serialised_db_value) const;
  SerialisedValue GetFromDb(const DbKey& db_key);
};


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_MERGE_POLICY_H_
