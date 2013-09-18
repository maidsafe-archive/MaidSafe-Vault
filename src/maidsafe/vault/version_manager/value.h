/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_VALUE_H_

#include <cstdint>
#include <string>

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

class VersionManagerValue {
 public:
  explicit VersionManagerValue(const std::string& serialised_version_manager_value);
  VersionManagerValue(uint32_t max_versions = 1, uint32_t max_branches = 1);
//  Commented by Mahmoud on 5 Sep. Requires fix in structureddata
//  VersionManagerValue(const VersionManagerValue& other);
//  VersionManagerValue(VersionManagerValue&& other);
  VersionManagerValue& operator=(VersionManagerValue other);
  std::string Serialise() const;

  void Put(const StructuredDataVersions::VersionName& old_version,
           const StructuredDataVersions::VersionName& new_version);
  std::vector<StructuredDataVersions::VersionName> Get() const;
  std::vector<StructuredDataVersions::VersionName> GetBranch(
      const StructuredDataVersions::VersionName& branch_tip) const;
  void DeleteBranchUntilFork(const StructuredDataVersions::VersionName& branch_tip);

  friend void swap(VersionManagerValue& lhs, VersionManagerValue& rhs);
  friend bool operator==(const VersionManagerValue& lhs, const VersionManagerValue& rhs);

 private:
  StructuredDataVersions structured_data_versions_;
};

// bool operator==(const VersionManagerValue& lhs, const VersionManagerValue& rhs);
void swap(VersionManagerValue& lhs, VersionManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
