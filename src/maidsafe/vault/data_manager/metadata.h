/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_METADATA_VALUE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_METADATA_VALUE_H_

#include <cstdint>
#include <set>
#include <vector>

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class Metadata {
 public:
  // This constructor reads the existing element or creates a new one if it doesn't already exist.
  Metadata(const DataNameVariant& data_name,
           ManagerDb<DataManager>* metadata_db,
           int32_t data_size);
  // This constructor reads the existing element or throws if it doesn't already exist.
  Metadata(const DataNameVariant& data_name, ManagerDb<DataManager>* metadata_db);
  // Should only be called once.
  void SaveChanges(ManagerDb<DataManager>* metadata_db);

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
