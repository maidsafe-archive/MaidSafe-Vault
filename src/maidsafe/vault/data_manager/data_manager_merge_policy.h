/* Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_METADATA_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_METADATA_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_element.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/data_manager/data_manager.h"


namespace maidsafe {

namespace vault {

typedef UnresolvedElement<DataManager> MetadataUnresolvedEntry;
typedef MetadataUnresolvedEntry MetadataResolvedEntry;

class MetadataMergePolicy {
 public:
  typedef MetadataUnresolvedEntry UnresolvedEntry;
  typedef MetadataResolvedEntry ResolvedEntry;
  typedef ManagerDb<DataManager> Database;

  explicit MetadataMergePolicy(ManagerDb<DataManager>* metadata_db);
  MetadataMergePolicy(MetadataMergePolicy&& other);
  MetadataMergePolicy& operator=(MetadataMergePolicy&& other);

 protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  ManagerDb<DataManager>* metadata_db_;

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
