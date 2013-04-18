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

#ifndef MAIDSAFE_VAULT_SYNC_H_
#define MAIDSAFE_VAULT_SYNC_H_

#include <map>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/unresolved_entry.h"
#include "maidsafe/vault/db.h"


namespace maidsafe {

namespace vault {
// Purpose of this object is to ensure enough nodes have agreed the message is valid

template<typename MergePolicy>
class Sync : public MergePolicy {
 public:
  explicit Sync(Db* db);
  Sync(Sync&& other);
  Sync& operator=(Sync&& other);
  void AddUnresolvedEntry(typename MergePolicy::UnresolvedEntry&,
                          const NodeId& node_id);

  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);
  // calling this WILL increment the sync counter and delete data that reaches the limit
  std::vector<typename MergePolicy::UnresolvedEntry> GetUnresolvedData();
  int32_t sync_counter() const { return sync_counter_; }
  void set_sync_counter(int32_t new_count) { sync_counter_ = new_count; }
 private:
  Sync(const Sync&);
  Sync& operator=(const Sync&);
  int32_t sync_counter_;
  typename std::vector<typename MergePolicy::UnresolvedEntry>::iterator FindUnresolved(
      const typename MergePolicy::UnresolvedEntry::Key& key);
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/sync-inl.h"

#endif  // MAIDSAFE_VAULT_SYNC_H_
