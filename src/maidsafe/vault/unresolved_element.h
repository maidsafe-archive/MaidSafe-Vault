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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_

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

template<typename ValueType>
struct UnresolvedElement {
  typedef std::pair<DataNameVariant, nfs::MessageAction> Key;
  typedef ValueType Value;

 private:
  template<typename Value>
  struct SerialisedUnresolvedEntryTag {};

 public:
  typedef TaggedValue<NonEmptyString, SerialisedUnresolvedEntryTag<Value>> serialised_type;

  struct MessageContent {
    NodeId peer_id;
    boost::optional<int32_t> entry_id;
    boost::optional<ValueType> value;
  };

  UnresolvedElement();
  explicit UnresolvedElement(const serialised_type& serialised_copy);
  UnresolvedElement(const UnresolvedElement& other);
  UnresolvedElement(UnresolvedElement&& other);
  UnresolvedElement& operator=(UnresolvedElement other);
  UnresolvedElement(const Key& key, const Value& value, const NodeId& sender_id);

  template<typename T>
  friend void swap(UnresolvedElement& lhs, UnresolvedElement& rhs) MAIDSAFE_NOEXCEPT;

  serialised_type Serialise() const;

  Key key;
  std::vector<MessageContent> messages_contents;
  int sync_counter;
  bool dont_add_to_db;
};

typedef UnresolvedElement<int32_t> MaidAccountUnresolvedEntry, PmidAccountUnresolvedEntry;

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/unresolved_element-inl.h"

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
