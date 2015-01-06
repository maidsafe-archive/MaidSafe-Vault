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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_VALUE_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_VALUE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/common/data_types/structured_data_versions.h"
#include "maidsafe/vault/version_handler/version_handler.h"

namespace maidsafe {

namespace vault {

class VersionHandlerValue {
 public:
  explicit VersionHandlerValue(const std::string& serialised_version_handler_value);
  VersionHandlerValue(uint32_t max_versions, uint32_t max_branches);  // BEFORE_RELEASE
  VersionHandlerValue(VersionHandlerValue&& other) MAIDSAFE_NOEXCEPT;
  VersionHandlerValue& operator=(VersionHandlerValue other);
  std::string Serialise() const;
  std::string Print() const;

  boost::optional<StructuredDataVersions::VersionName> Put(
      const StructuredDataVersions::VersionName& old_version,
      const StructuredDataVersions::VersionName& new_version);
  std::vector<StructuredDataVersions::VersionName> Get() const;
  std::vector<StructuredDataVersions::VersionName> GetBranch(
      const StructuredDataVersions::VersionName& branch_tip) const;
  void DeleteBranchUntilFork(const StructuredDataVersions::VersionName& branch_tip);

  friend void swap(VersionHandlerValue& lhs, VersionHandlerValue& rhs);

 private:
  typedef std::unique_ptr<StructuredDataVersions> StructuredDataVersionsPtr;
  VersionHandlerValue(const VersionHandlerValue&) = delete;

  StructuredDataVersionsPtr structured_data_versions_;
};

void swap(VersionHandlerValue& lhs, VersionHandlerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_VALUE_H_
