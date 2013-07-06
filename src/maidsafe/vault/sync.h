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

#include "boost/optional/optional.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"


namespace maidsafe {

namespace vault {

// The purpose of this class is to ensure enough peers have agreed a given request is valid before
// recording the corresponding action to an AccountDb/ManagerDb.  This should ensure that all peers
// hold similar, if not identical databases.
template<typename MergePolicy>
class Sync : public MergePolicy {
 public:
  Sync(typename MergePolicy::Database* database, const NodeId& this_node_id);
  Sync(Sync&& other);
  Sync& operator=(Sync&& other);
  // This is called when receiving a Sync message from a peer or this node.  If the
  // entry becomes resolved it is returned.
  template<nfs::MessageAction action>
  typename MergePolicy::ResolvedEntry AddUnresolvedEntry(
      const typename MergePolicy::UnresolvedEntry<action>& entry);
  // This is called directly once an action has been decided as valid in the MAHolder, but before
  // syncing the unresolved entry to the peers.  This won't resolve the entry (even if it's the last
  // one we're waiting for) so that 'GetUnresolvedData()' will return this one, allowing us to then
  // sync it to our peers.
  template<nfs::MessageAction action>
  void AddLocalEntry(const typename MergePolicy::UnresolvedEntry<action>& entry);
  // This is called if, during an ongoing account transfer, another churn event occurs.  The
  // old node's ID is replaced throughout the list of unresolved entries with the new node's ID.
  // The new node's ID is also applied to any entries which didn't contain the old one, in the
  // expectation that the old node would have eventually supplied the message.
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);
  void ReplaceNode(const typename MergePolicy::DbKey& db_key,
                   const NodeId& old_node,
                   const NodeId& new_node);
  // This returns all unresoved entries containing this node's ID.  Each returned entry is provided
  // with just this node's ID inserted, even if the master copy has several other peers' IDs.
  // Calling this will increment the sync counter and delete entries that reach the
  // 'sync_counter_max_' limit.  Entries which are resolved by all peers (i.e. have 4 messages) are
  // also pruned here.
  std::vector<typename MergePolicy::UnresolvedEntryVariant> GetUnresolvedData(
      bool increment_sync_attempts = true);
  size_t GetUnresolvedCount() const { return MergePolicy::unresolved_data_.size(); }
  size_t GetUnresolvedCount(const typename MergePolicy::DbKey& db_key) const;

 private:
  Sync(const Sync&);
  Sync& operator=(const Sync&);
  template<nfs::MessageAction action>
  boost::optional<typename MergePolicy::ResolvedEntry> AddEntry(
      const typename MergePolicy::UnresolvedEntry<action>& entry,
      bool merge);
  typename MergePolicy::UnresolvedEntriesItr ReplaceNodeAndIncrementItr(
      typename MergePolicy::UnresolvedEntriesItr entries_itr,
      const NodeId& old_node,
      const NodeId& new_node);
  template<nfs::MessageAction action>
  bool CanBeErased(const typename MergePolicy::UnresolvedEntry<action>& entry) const;

  int32_t sync_counter_max_;
  NodeId this_node_id_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/sync-inl.h"

#endif  // MAIDSAFE_VAULT_SYNC_H_
