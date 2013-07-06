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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_H_

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "boost/optional/optional.hpp"
#include "boost/variant/variant.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/unresolved_entry_core_fields.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"


namespace maidsafe {

namespace vault {

template<>
struct ActionAttributes<MaidManager, nfs::MessageAction::kPut> {
  NodeId peer_id;
  UnresolvedEntryId entry_id;
  typename MaidManager::Cost cost;
};

template<>
struct ActionAttributes<MaidManager, nfs::MessageAction::kDelete> {
  NodeId peer_id;
  UnresolvedEntryId entry_id;
};

template<>
struct ActionAttributes<MaidManager, nfs::MessageAction::kRegisterPmid> {
  NodeId peer_id;
  UnresolvedEntryId entry_id;
  PmidName pmid_name;
};

template<>
struct ActionAttributes<MaidManager, nfs::MessageAction::kUnregisterPmid> {
  NodeId peer_id;
  UnresolvedEntryId entry_id;
  PmidName pmid_name;
};


template<nfs::MessageAction action>
struct MaidAccountUnresolvedEntry {
 public:
  MaidAccountUnresolvedEntry();
  MaidAccountUnresolvedEntry(const MaidAccountUnresolvedEntry& other);
  MaidAccountUnresolvedEntry(MaidAccountUnresolvedEntry&& other);
  MaidAccountUnresolvedEntry& operator=(MaidAccountUnresolvedEntry other);
  MaidAccountUnresolvedEntry(const MaidManager::DbKey& key,
                             const ActionAttributes<MaidManager, action>& action_attributes);

  explicit MaidAccountUnresolvedEntry(const nfs::Message& message);
  nfs::Message ToMessage() const;

  UnresolvedEntryCoreFields<MaidManager, action> core;
};

template<>
struct MaidAccountUnresolvedEntry<nfs::MessageAction::kPut> {
 public:
  MaidAccountUnresolvedEntry();
  MaidAccountUnresolvedEntry(const MaidAccountUnresolvedEntry& other);
  MaidAccountUnresolvedEntry(MaidAccountUnresolvedEntry&& other);
  MaidAccountUnresolvedEntry& operator=(MaidAccountUnresolvedEntry other);
  MaidAccountUnresolvedEntry(const MaidManager::DbKey& key,
                             const ActionAttributes<MaidManager,
                                                    nfs::MessageAction::kPut>& action_attributes);

  explicit MaidAccountUnresolvedEntry(const nfs::Message& message);
  nfs::Message ToMessage() const;

  UnresolvedEntryCoreFields<MaidManager, nfs::MessageAction::kPut> core;
  bool dont_add_to_db;
};

template<nfs::MessageAction action>
void swap(MaidAccountUnresolvedEntry<action>& lhs,
          MaidAccountUnresolvedEntry<action>& rhs) MAIDSAFE_NOEXCEPT;

typedef boost::variant<
    MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>,
    MaidAccountUnresolvedEntry<nfs::MessageAction::kDelete>,
    MaidAccountUnresolvedEntry<nfs::MessageAction::kRegisterPmid>,
    MaidAccountUnresolvedEntry<nfs::MessageAction::kUnregisterPmid>>
        MaidAccountUnresolvedEntryVariant;

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_manager/unresolved_entry-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_H_
