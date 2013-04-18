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
#include <set>
#include <utility>

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

struct MaidAndPmidUnresolvedAction {
  typedef std::pair<DataNameVariant, nfs::MessageAction> Key;
  typedef int32_t Value;
  typedef TaggedValue<NonEmptyString,
                      struct SerialisedMaidAndPmidUnresolvedActionTag> serialised_type;

  MaidAndPmidUnresolvedAction();
  explicit MaidAndPmidUnresolvedAction(const serialised_type& serialised_copy);
  MaidAndPmidUnresolvedAction(const MaidAndPmidUnresolvedAction& other);
  MaidAndPmidUnresolvedAction(MaidAndPmidUnresolvedAction&& other);
  MaidAndPmidUnresolvedAction& operator=(MaidAndPmidUnresolvedAction other);
  MaidAndPmidUnresolvedAction(const Key& data_name_and_action, Value cost);
  friend void swap(MaidAndPmidUnresolvedAction& lhs,
                   MaidAndPmidUnresolvedAction& rhs) MAIDSAFE_NOEXCEPT;
  serialised_type Serialise() const;

  Key data_name_and_action;
  Value cost;
  std::set<NodeId> peers;
  int sync_counter;
  bool dont_add_to_db;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_
