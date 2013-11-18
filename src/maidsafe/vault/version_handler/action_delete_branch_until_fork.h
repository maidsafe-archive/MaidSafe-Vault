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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_

#include <string>

#include "maidsafe/data_types/structured_data_versions.h"

#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {

class VersionHandlerValue;

struct ActionVersionHandlerDeleteBranchUntilFork {
  explicit ActionVersionHandlerDeleteBranchUntilFork(const std::string& serialised_action);
  ActionVersionHandlerDeleteBranchUntilFork(const ActionVersionHandlerDeleteBranchUntilFork& other);
  ActionVersionHandlerDeleteBranchUntilFork(
      const ActionVersionHandlerDeleteBranchUntilFork&& other);
  ActionVersionHandlerDeleteBranchUntilFork(
      const StructuredDataVersions::VersionName& version_name);
  detail::DbAction operator()(std::unique_ptr<VersionHandlerValue>& value);

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kDeleteBranchUntilForkRequest;

  friend bool operator==(const ActionVersionHandlerDeleteBranchUntilFork& lhs,
                         const ActionVersionHandlerDeleteBranchUntilFork& rhs);

 private:
  ActionVersionHandlerDeleteBranchUntilFork();
  ActionVersionHandlerDeleteBranchUntilFork& operator=(
      ActionVersionHandlerDeleteBranchUntilFork other);
  StructuredDataVersions::VersionName version_name;
};

bool operator==(const ActionVersionHandlerDeleteBranchUntilFork& lhs,
                const ActionVersionHandlerDeleteBranchUntilFork& rhs);
bool operator!=(const ActionVersionHandlerDeleteBranchUntilFork& lhs,
                const ActionVersionHandlerDeleteBranchUntilFork& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_DELETE_BRANCH_UNTIL_FORK_H_
