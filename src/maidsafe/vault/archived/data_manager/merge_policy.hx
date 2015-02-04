/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_METADATA_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_METADATA_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
//#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/data_manager/data_manager.h"


namespace maidsafe {

namespace vault {


class MetadataMergePolicy {
 public:
  typedef UnresolvedAction UnresolvedEntry;
  typedef UnresolvedAction ResolvedEntry;
  typedef ManagerDb<DataManagerKey, DataManagerValue> Database;

  explicit MetadataMergePolicy(ManagerDb<DataManagerKey, DataManagerValue>* metadata_db);
  MetadataMergePolicy(MetadataMergePolicy&& other);
  MetadataMergePolicy& operator=(MetadataMergePolicy&& other);

 protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  ManagerDb<DataManagerKey, DataManagerValue>* metadata_db_;

 private:
  MetadataMergePolicy(const MetadataMergePolicy&);
  MetadataMergePolicy& operator=(const MetadataMergePolicy&);

  void MergePut(const DataNameVariant& data_name, int data_size);
  void MergeDelete(const DataNameVariant& data_name, int data_size);
  int GetDataSize(const UnresolvedEntry& unresolved_entry) const;
  std::vector<UnresolvedEntry> MergeRecordTransfer(const UnresolvedEntry& unresolved_entry);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_METADATA_MERGE_POLICY_H_
