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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/unresolved_entry.h"


namespace maidsafe {

namespace vault {

class PmidAccountMergePolicy {
 public:
  explicit PmidAccountMergePolicy(AccountDb* account_db);
  PmidAccountMergePolicy(PmidAccountMergePolicy&& other);
  PmidAccountMergePolicy& operator=(PmidAccountMergePolicy&& other);
  // This flags a "Put" entry in 'unresolved_data_' as not to be added to the db.
  template<typename Data>
  int32_t AllowDelete(const typename Data::name_type& name);

 protected:
  typedef MaidAndPmidUnresolvedEntry UnresolvedEntry;
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  AccountDb* account_db_;

 private:
  typedef TaggedValue<int32_t, struct SizeTag> Size;
  typedef TaggedValue<int32_t, struct CountTag> Count;

  PmidAccountMergePolicy(const PmidAccountMergePolicy&);
  PmidAccountMergePolicy& operator=(const PmidAccountMergePolicy&);

  void MergePut(const DataNameVariant& data_name,
                UnresolvedEntry::Value size,
                const NonEmptyString& serialised_db_value);
  void MergeDelete(const DataNameVariant& data_name, const NonEmptyString& serialised_db_value);
  NonEmptyString SerialiseDbValue(Size size) const;
  Size ParseDbValue(NonEmptyString serialised_db_value) const;
  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

template<typename Data>
int32_t PmidAccountMergePolicy::AllowDelete(const typename Data::name_type& name) {
  auto serialised_db_value(GetFromDb(name));
  Size size(0);
  Count current_count(0);
  if (serialised_db_value.IsInitialised())
    return ParseDbValue(serialised_db_value);

  DataNameVariant name_as_variant(name);
  auto itr(std::begin(unresolved_data_));
  auto last_put_still_to_be_added_to_db(std::end(unresolved_data_));
  int32_t pending_puts(0), pending_deletes(0);

  while (itr != std::end(unresolved_data_)) {
    if ((*itr).data_name_and_action.first == name_as_variant) {
      if ((*itr).data_name_and_action.second == nfs::MessageAction::kPut) {
        if ((*itr).dont_add_to_db) {
          // A delete request must have been applied for this to be true, but it will (correctly)
          // silently fail when it comes to merging since this put request will not have been
          // added to the account_db.
          --pending_deletes;
        } else {
          ++pending_puts;
          last_put_still_to_be_added_to_db = itr;
          if (size != 0)
            size.data = (*itr).cost;
        }
      } else {
        assert((*itr).data_name_and_action.second == nfs::MessageAction::kDelete);
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

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_MERGE_POLICY_H_
