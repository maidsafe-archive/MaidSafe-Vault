/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_METADATA_VALUE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_METADATA_VALUE_H_

#include <cstdint>
#include <set>
#include <vector>

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/db.h"


namespace maidsafe {

namespace vault {

class Metadata {
 public:
  // This constructor reads the existing element or creates a new one if it doesn't already exist.
  Metadata(const DataNameVariant& data_name,
           Db<DataManager::Key, DataManager::Value>* metadata_db,
           int32_t data_size);
  // This constructor reads the existing element or throws if it doesn't already exist.
  Metadata(const DataNameVariant& data_name,
           Db<DataManager::Key, DataManager::Value>* metadata_db);
  // Should only be called once.
  void SaveChanges(Db<DataManager::Key, DataManager::Value>* metadata_db);

  DataNameVariant data_name_;
  DataManagerValue value_;
  on_scope_exit strong_guarantee_;

 private:
  Metadata();
  Metadata(const Metadata&);
  Metadata& operator=(const Metadata&);
  Metadata(Metadata&&);
  Metadata& operator=(Metadata&&);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_METADATA_VALUE_H_
