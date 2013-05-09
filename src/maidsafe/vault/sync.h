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

#include <cstdint>
#include <vector>

#include "maidsafe/common/node_id.h"


namespace maidsafe {

namespace vault {

class AccountDb;

// The purpose of this class is to ensure enough peers have agreed a given request is valid before
// recording the corresponding action to an AccountDb.  This should ensure that all peers hold
// similar, if not identical databases.
template<typename MergePolicy>
class Sync : public MergePolicy {
 public:
  Sync(AccountDb* account_db, const NodeId& this_node_id);
  Sync(Sync&& other);
  Sync& operator=(Sync&& other);
  // This is called when receiving a Sync message from a peer or this node.  Returns true if the
  // entry becomes resolved.
  bool AddUnresolvedEntry(const typename MergePolicy::UnresolvedEntry& entry);
  // This is called directly once an action has been decided as valid in the MAHolder, but before
  // syncing the unresolved entry to the peers.  This won't resolve the entry (even if it's the last
  // one we're waiting for) so that 'GetUnresolvedData()' will return this one, allowing us to then
  // sync it to our peers.
  void AddLocalEntry(const typename MergePolicy::UnresolvedEntry& entry);
  // Returns true if the entry becomes resolved.
  bool AddAccountTransferRecord(const typename MergePolicy::UnresolvedEntry& entry,
                                bool all_account_transfers_received);
  // This is called if, during an ongoing account transfer, another churn event occurs.  The
  // old node's ID is replaced throughout the list of unresolved entries with the new node's ID.
  // The new node's ID is also applied to any entries which didn't contain the old one, in the
  // expectation that the old node would have eventually supplied the message.
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);
  // This returns all unresoved entries containing this node's ID.  Each returned entry is provided
  // with just this node's ID inserted, even if the master copy has several other peers' IDs.
  std::vector<typename MergePolicy::UnresolvedEntry> GetUnresolvedData();
  size_t GetUnresolvedCount() const { return MergePolicy::unresolved_data_.size(); }
  // Calling this will increment the sync counter and delete entries that reach the
  // 'sync_counter_max_' limit.  Entries which are resolved by all peers (i.e. have 4 messages) are
  // also pruned here.
  void IncrementSyncAttempts();

 private:
  Sync(const Sync&);
  Sync& operator=(const Sync&);
  bool AddEntry(const typename MergePolicy::UnresolvedEntry& entry, bool merge);
  bool CanBeErased(const typename MergePolicy::UnresolvedEntry& entry) const;

  int32_t sync_counter_max_;
  NodeId this_node_id_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/sync-inl.h"

#endif  // MAIDSAFE_VAULT_SYNC_H_
