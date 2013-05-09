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
  NonEmptyString SerialiseDbValue(DbValue db_value) const;
  DbValue ParseDbValue(NonEmptyString serialised_db_value) const;
  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

template<typename Data>
int32_t MetadataMergePolicy::AllowDelete(const typename Data::name_type& name) {
  auto serialised_db_value(GetFromDb(name));
  Count current_count(0);
  AverageCost size(0);
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    assert(current_values.second.data > 0);
    current_count = current_values.second;
    size.data = current_values.first;
  }

  DataNameVariant name_as_variant(name);
  auto itr(std::begin(unresolved_data_));
  auto last_put_still_to_be_added_to_db(std::end(unresolved_data_));
  int32_t pending_puts(0), pending_deletes(0);

  while (itr != std::end(unresolved_data_)) {
    if ((*itr).key.first == name_as_variant) {
      if ((*itr).key.second == nfs::MessageAction::kPut) {
        if ((*itr).dont_add_to_db) {
          // A delete request must have been applied for this to be true, but it will (correctly)
          // silently fail when it comes to merging since this put request will not have been
          // added to the account_db.
          --pending_deletes;
        } else {
          ++pending_puts;
          last_put_still_to_be_added_to_db = itr;
          if (size != 0 && (*itr).messages_contents.front().value) {
              // TODO(dirvine) we should average these results rather than taking the first
            size.data = *(*itr).messages_contents.front().value;
          }
        }
      } else {
        assert((*itr).key.second == nfs::MessageAction::kDelete);
        ++pending_deletes;
      }
    }
    ++itr;
  }

  if (current_count <= pending_deletes &&
      last_put_still_to_be_added_to_db != std::end(unresolved_data_)) {
    (*last_put_still_to_be_added_to_db).dont_add_to_db = true;
  }

  if (current_count + pending_puts <= pending_deletes)
    size.data = 0;
  return size.data;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MERGE_POLICY_H_
