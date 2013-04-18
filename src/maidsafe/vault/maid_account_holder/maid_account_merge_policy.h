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
  template<typename Data>
  bool AllowDelete(const typename Data::name_type& name) const;

 protected:
  typedef MaidAndPmidUnresolvedAction UnresolvedAction;
  // If Deleting and the DB doesn't contain the element, throws 'CommonErrors::no_such_element'.
  // Check in 'unresolved_data_' for a corresponding (out of sequence) Put request.
  void Merge(const UnresolvedAction::Key& data_name_and_action, UnresolvedAction::Value cost);

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
};

template<typename Data>
bool MaidAccountMergePolicy::AllowDelete(const typename Data::name_type& name) const {
                                                                     //todo - check if throws if not exists
  auto serialised_db_value(db_->Get(name));
  Count current_count(0);
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    assert(current_values.second.data > 0);
    current_count = current_values.second;
  }

  DataNameVariant name_as_variant(name);
  for (const auto& unresolved_action : unresolved_data_) {
    if (unresolved_action.data_name_and_action.first == name_as_variant) {
                                                                     //todo - should this only be for our own NodeId?
      if (unresolved_action.data_name_and_action.second == nfs::MessageAction::kPut) {
        ++current_count.data;
      } else {
        assert(data_name_and_action.second == nfs::MessageAction::kDelete);
        --current_count.data;
      }
    }
  }

  return current_count > 0;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_MERGE_POLICY_H_
