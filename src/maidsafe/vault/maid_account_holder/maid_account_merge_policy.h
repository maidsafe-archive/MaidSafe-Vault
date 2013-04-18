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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/db.h"
#include "maidsafe/vault/unresolved_action.h"


namespace maidsafe {

namespace vault {

class MaidAccountMergePolicy {
 public:
  explicit MaidAccountMergePolicy(Db* db);
  MaidAccountMergePolicy(MaidAccountMergePolicy&& other);
  MaidAccountMergePolicy& operator=(MaidAccountMergePolicy&& other);
  // This flags a "Put" entry in 'unresolved_data_' as not to be added to the db.
  template<typename Data>
  bool AllowDelete(const typename Data::name_type& name);

 protected:
  typedef MaidAndPmidUnresolvedAction UnresolvedAction;
  void Merge(const UnresolvedAction& unresolved_action);

  std::vector<UnresolvedAction> unresolved_data_;
  Db* db_;

 private:
  typedef TaggedValue<int32_t, struct AverageCostTag> AverageCost;
  typedef TaggedValue<int32_t, struct CountTag> Count;
  typedef std::pair<AverageCost, Count> DbValue;

  MaidAccountMergePolicy(const MaidAccountMergePolicy&);
  MaidAccountMergePolicy& operator=(const MaidAccountMergePolicy&);

  void MergePut(const DataNameVariant& data_name,
                UnresolvedAction::Value cost,
                const NonEmptyString& serialised_db_value);
  void MergeDelete(const DataNameVariant& data_name, const NonEmptyString& serialised_db_value);
  NonEmptyString SerialiseDbValue(DbValue db_value) const;
  DbValue ParseDbValue(NonEmptyString serialised_db_value) const;
  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

template<typename Data>
bool MaidAccountMergePolicy::AllowDelete(const typename Data::name_type& name) {
  auto serialised_db_value(GetFromDb(name));
  Count current_count(0);
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    assert(current_values.second.data > 0);
    current_count = current_values.second;
  }

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
          // added to the db.
          --pending_deletes;
        } else {
          ++pending_puts;
          last_put_still_to_be_added_to_db = itr;
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
    (last_put_still_to_be_added_to_db).dont_add_to_db = true;
  }

  return current_count + pending_puts > pending_deletes;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_MERGE_POLICY_H_
