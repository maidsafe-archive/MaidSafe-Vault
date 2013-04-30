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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "boost/optional/optional.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

template<nfs::Persona persona>
struct UnresolvedEntry {};

template<>
struct UnresolvedEntry<nfs::Persona::kMaidAccountHolder> {
  typedef int32_t Value;
  struct MessageContent {
    NodeId node_id;
    boost::optional<int32_t> entry_id;
    boost::optional<Value> value;
  };
  typedef TaggedValue<NonEmptyString, struct SerialisedMaidUnresolvedEntryTag> serialised_type;

  UnresolvedEntry();
  explicit UnresolvedEntry(const serialised_type& serialised_copy);
  UnresolvedEntry(const UnresolvedEntry& other);
  UnresolvedEntry(UnresolvedEntry&& other);
  UnresolvedEntry& operator=(UnresolvedEntry other);
  UnresolvedEntry(const DataNameVariant& data_name,
                  nfs::MessageAction action,
                  Value cost,
                  const NodeId& sender_id);
  friend void swap(UnresolvedEntry& lhs, UnresolvedEntry& rhs) MAIDSAFE_NOEXCEPT;
  serialised_type Serialise() const;

  std::pair<DataNameVariant, nfs::MessageAction> key;
  std::vector<MessageContent> message_content;
  int sync_counter;
  bool dont_add_to_db;
};

template<>
struct UnresolvedEntry<nfs::Persona::kPmidAccountHolder> {
  typedef int32_t Value;
  struct MessageContent {
    NodeId node_id;
    boost::optional<int32_t> entry_id;
    boost::optional<Value> value;
  };
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidUnresolvedEntryTag> serialised_type;

  UnresolvedEntry();
  explicit UnresolvedEntry(const serialised_type& serialised_copy);
  UnresolvedEntry(const UnresolvedEntry& other);
  UnresolvedEntry(UnresolvedEntry&& other);
  UnresolvedEntry& operator=(UnresolvedEntry other);
  UnresolvedEntry(const DataNameVariant& data_name,
                  nfs::MessageAction action,
                  Value cost,
                  const NodeId& sender_id);
  friend void swap(UnresolvedEntry& lhs, UnresolvedEntry& rhs) MAIDSAFE_NOEXCEPT;
  serialised_type Serialise() const;
  std::pair<DataNameVariant, nfs::MessageAction> key;
  std::vector<MessageContent> message_content;
  int sync_counter;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/unresolved_entry-inl.h"

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_
