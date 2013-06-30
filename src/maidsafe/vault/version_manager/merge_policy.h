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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/unresolved_element.h"
#include "maidsafe/vault/version_manager/version_manager.h"
#include "maidsafe/vault/version_manager/key.h"
#include "maidsafe/vault/version_manager/unresolved_entry_value.h"


namespace maidsafe {

namespace vault {

class VersionManagerMergePolicy {
 public:
  typedef VersionManagerUnresolvedEntry UnresolvedEntry;
  typedef VersionManagerResolvedEntry ResolvedEntry;
  typedef VersionManager::DbKey DbKey;
  typedef ManagerDb<VersionManager> Database;

  explicit VersionManagerMergePolicy(ManagerDb<VersionManager>* db);
  VersionManagerMergePolicy(VersionManagerMergePolicy&& other);
  VersionManagerMergePolicy& operator=(VersionManagerMergePolicy&& other);

 protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  ManagerDb<VersionManager>* db_;

 private:
  VersionManagerMergePolicy(const VersionManagerMergePolicy&);
  VersionManagerMergePolicy& operator=(const VersionManagerMergePolicy&);

  void MergePut(const DbKey& key,
                const StructuredDataVersions::VersionName& new_value,
                const StructuredDataVersions::VersionName& old_value);

  void MergeDeleteBranchUntilFork(const DbKey& key,
                                  const StructuredDataVersions::VersionName& tot);
  void MergeDelete(const DbKey& key);

  void MergeAccountTransfer(const DbKey& key, const StructuredDataVersions& data_version);
};


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_MERGE_POLICY_H_
