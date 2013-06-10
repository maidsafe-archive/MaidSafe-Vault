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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_H_

#include <cstdint>
#include <utility>

#include "maidsafe/common/types.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/structured_data_manager/structured_data_key.h"
#include "maidsafe/vault/structured_data_manager/structured_data_unresolved_entry_value.h"


namespace maidsafe {

namespace nfs {

template<>
struct PersonaTypes<Persona::kStructuredDataManager> {
  typedef DataNameVariant RecordName;
  typedef vault::StructuredDataKey DbKey;
  typedef StructuredDataVersions DbValue;
  typedef std::pair<DbKey, MessageAction> UnresolvedEntryKey;
  typedef vault::StructuredDataUnresolvedEntryValue UnresolvedEntryValue;
  static const Persona persona = Persona::kStructuredDataManager;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kStructuredDataManager> StructuredDataManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_H_
