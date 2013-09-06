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
