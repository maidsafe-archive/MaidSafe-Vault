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
#include <tuple>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/db.h"


namespace maidsafe {

namespace vault {

class MaidAccountMergePolicy {
 public:
  explicit MaidAccountMergePolicy(Db* db);
  MaidAccountMergePolicy(MaidAccountMergePolicy&& other);
  MaidAccountMergePolicy& operator=(MaidAccountMergePolicy&& other);

 protected:
  typedef std::tuple<DataNameVariant,
                     NonEmptyString,
                     nfs::MessageAction,
                     std::set<NodeId>> UnresolvedEntry;
  MaidAccountMergePolicy(const MaidAccountMergePolicy&);
  MaidAccountMergePolicy& operator=(const MaidAccountMergePolicy&);
  void Merge(const DataNameVariant& key,
             const NonEmptyString& value,
             nfs::MessageAction message_action);

  std::vector<UnresolvedEntry> unresolved_data_;
  Db* db_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_MERGE_POLICY_H_
