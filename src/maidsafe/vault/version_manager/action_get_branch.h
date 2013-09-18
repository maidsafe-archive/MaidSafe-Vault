/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_GET_BRANCH_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_GET_BRANCH_H_

#include <string>

#include "maidsafe/data_types/structured_data_versions.h"

#include "maidsafe/vault/version_manager/version_manager.h"
#include "maidsafe/vault/version_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionVersionManagerGetBranch {
  explicit ActionVersionManagerGetBranch(const std::string& serialised_action);
  explicit ActionVersionManagerGetBranch(const StructuredDataVersions::VersionName& version_name);

  void operator()(boost::optional<VersionManagerValue>& value,
                  std::vector<StructuredDataVersions::VersionName>& version_names) const;

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kGetBranchRequest;

 private:
  ActionVersionManagerGetBranch();
  ActionVersionManagerGetBranch& operator=(ActionVersionManagerGetBranch other);
  StructuredDataVersions::VersionName version_name;
};

bool operator==(const ActionVersionManagerGetBranch& lhs, const ActionVersionManagerGetBranch& rhs);
bool operator!=(const ActionVersionManagerGetBranch& lhs, const ActionVersionManagerGetBranch& rhs);


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_ACTION_GET_BRANCH_H_
