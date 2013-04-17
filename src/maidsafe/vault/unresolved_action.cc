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


#include "maidsafe/vault/unresolved_action.h"


namespace maidsafe {

namespace vault {

MaidAndPmidUnresolvedAction::MaidAndPmidUnresolvedAction()
    : data_name_and_action(),
      cost(0),
      peers() {}

MaidAndPmidUnresolvedAction::MaidAndPmidUnresolvedAction(const MaidAndPmidUnresolvedAction& other)
    : data_name_and_action(other.data_name_and_action),
      cost(other.cost),
      peers(other.peers) {}

MaidAndPmidUnresolvedAction::MaidAndPmidUnresolvedAction(MaidAndPmidUnresolvedAction&& other)
    : data_name_and_action(std::move(other.data_name_and_action)),
      cost(std::move(other.cost)),
      peers(std::move(other.peers)) {}

MaidAndPmidUnresolvedAction& MaidAndPmidUnresolvedAction::operator=(
    MaidAndPmidUnresolvedAction other) {
  swap(*this, other);
  return *this;
}

MaidAndPmidUnresolvedAction::MaidAndPmidUnresolvedAction(const Key& data_name_and_action_in,
                                                         Value cost_in)
    : data_name_and_action(data_name_and_action_in),
      cost(cost_in),
      peers() {}

void swap(MaidAndPmidUnresolvedAction& lhs, MaidAndPmidUnresolvedAction& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name_and_action, rhs.data_name_and_action);
  swap(lhs.cost, rhs.cost);
  swap(lhs.peers, rhs.peers);
}

}  // namespace vault

}  // namespace maidsafe
