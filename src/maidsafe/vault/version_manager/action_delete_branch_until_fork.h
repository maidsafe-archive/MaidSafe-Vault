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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_

#include <string>

#include "maidsafe/data_types/structured_data_versions.h"

#include "maidsafe/vault/version_manager/version_manager.h"
#include "maidsafe/vault/version_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionDeleteBranchUntilFork {
  explicit ActionDeleteBranchUntilFork(const std::string& serialised_action);
  explicit ActionDeleteBranchUntilFork(const StructuredDataVersions::VersionName& version_name);

  void operator()(boost::optional<VersionManagerValue> value) const;

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kDeleteBranchUntilFork;

  friend bool operator==(const ActionDeleteBranchUntilFork& lhs,
                         const ActionDeleteBranchUntilFork& rhs);

 private:
  ActionDeleteBranchUntilFork();
  ActionDeleteBranchUntilFork& operator=(ActionDeleteBranchUntilFork other);
  StructuredDataVersions::VersionName version_name;
};

bool operator==(const ActionDeleteBranchUntilFork& lhs, const ActionDeleteBranchUntilFork& rhs);
bool operator!=(const ActionDeleteBranchUntilFork& lhs, const ActionDeleteBranchUntilFork& rhs);


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_
