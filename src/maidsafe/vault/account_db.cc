/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/


#include "maidsafe/vault/account_db.h"

#include <vector>

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/vault/db.h"

namespace maidsafe {
namespace vault {

AccountDb::AccountDb(Db& db)
  : db_(db),
    account_id_(db.RegisterAccount()) {
}

AccountDb::~AccountDb() {
  db_.UnRegisterAccount(account_id_);
}

void AccountDb::Put(const Db::KVPair& key_value_pair) {
  db_.Put(account_id_, key_value_pair);
}

void AccountDb::Delete(const DataNameVariant& key) {
  db_.Delete(account_id_, key);
}

NonEmptyString AccountDb::Get(const DataNameVariant& key) {
  return db_.Get(account_id_, key);
}

std::vector<Db::KVPair> AccountDb::Get() {
  return db_.Get(account_id_);
}

}  // namespace vault
}  // namespace maidsafe
