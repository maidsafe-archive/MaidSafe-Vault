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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_

#include "boost/optional.hpp"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

struct StructuredDataKey {
  nfs::PersonaId peer_type;
  DataNameVariant data_name;
  nfs::MessageAction action;
};

struct StructuredDataDbKey {
  explicit StructuredDataDbKey(StructuredDataKey& structured_data_key) :
    peer_type(structured_data_key.peer_type),
    data_name(structured_data_key.data_name) {}
  nfs::PersonaId peer_type;
  DataNameVariant data_name;
};


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_
