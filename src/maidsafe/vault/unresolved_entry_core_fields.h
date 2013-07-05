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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_H_

#include <cstdint>
#include <vector>

#include "maidsafe/common/config.h"


namespace maidsafe {

namespace vault {

typedef int32_t UnresolvedEntryId;

template<typename PersonaTypes, nfs::MessageAction action>
struct ActionAttributes;

template<typename PersonaTypes, nfs::MessageAction action>
struct UnresolvedEntryCoreFields {
  UnresolvedEntryCoreFields();
  UnresolvedEntryCoreFields(const UnresolvedEntryCoreFields& other);
  UnresolvedEntryCoreFields(UnresolvedEntryCoreFields&& other);
  UnresolvedEntryCoreFields& operator=(UnresolvedEntryCoreFields other);
  UnresolvedEntryCoreFields(
      const typename PersonaTypes::DbKey& key,
      const ActionAttributes<PersonaTypes, action>& sender_action_attributes);

  typename PersonaTypes::DbKey db_key;
  std::vector<ActionAttributes<PersonaTypes, action>> action_attributes;
  int sync_attempts;
};

template<typename PersonaTypes, nfs::MessageAction action>
void swap(UnresolvedEntryCoreFields<PersonaTypes, action>& lhs,
          UnresolvedEntryCoreFields<PersonaTypes, action>& rhs) MAIDSAFE_NOEXCEPT;

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/unresolved_entry_core_fields-inl.h"

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ENTRY_CORE_FIELDS_H_
