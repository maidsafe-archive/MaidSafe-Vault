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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_INL_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_INL_H_

#include <utility>


namespace maidsafe {

namespace vault {

template<typename PersonaTypes, nfs::MessageAction action>
UnresolvedEntryCoreFields<PersonaTypes, action>::UnresolvedEntryCoreFields()
    : db_key(),
      action_attributes(),
      sync_attempts(0) {}

template<typename PersonaTypes, nfs::MessageAction action>
UnresolvedEntryCoreFields<PersonaTypes, action>::UnresolvedEntryCoreFields(
    const UnresolvedEntryCoreFields& other)
        : db_key(other.key),
          action_attributes(other.action_attributes),
          sync_attempts(other.sync_attempts) {}

template<typename PersonaTypes, nfs::MessageAction action>
UnresolvedEntryCoreFields<PersonaTypes, action>::UnresolvedEntryCoreFields(
    UnresolvedEntryCoreFields&& other)
        : db_key(std::move(other.key)),
          action_attributes(std::move(other.action_attributes)),
          sync_attempts(std::move(other.sync_attempts)) {}

template<typename PersonaTypes, nfs::MessageAction action>
UnresolvedEntryCoreFields<PersonaTypes, action>&
    UnresolvedEntryCoreFields<PersonaTypes, action>::operator=(UnresolvedEntryCoreFields other) {
  swap(*this, other);
  return *this;
}

template<typename PersonaTypes, nfs::MessageAction action>
UnresolvedEntryCoreFields<PersonaTypes, action>::UnresolvedEntryCoreFields(
    const typename PersonaTypes::DbKey& key,
    const ActionAttributes<PersonaTypes, action>& sender_action_attributes)
        : db_key(key),
          action_attributes(1, sender_action_attributes),
          sync_attempts(0) {}

template<typename PersonaTypes, nfs::MessageAction action>
void swap(UnresolvedEntryCoreFields<PersonaTypes, action>& lhs,
          UnresolvedEntryCoreFields<PersonaTypes, action>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.key, rhs.key);
  swap(lhs.action_attributes, rhs.action_attributes);
  swap(lhs.sync_attempts, rhs.sync_attempts);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_INL_H_
