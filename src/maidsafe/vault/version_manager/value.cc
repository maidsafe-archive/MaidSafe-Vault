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

#include "maidsafe/vault/version_manager/value.h"

#include <functional>
#include <limits>
#include <utility>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/version_manager/version_manager.pb.h"

namespace maidsafe {

namespace vault {

//  Commented by Mahmoud on 5 Sep. Requires fix in structureddata
// VersionManagerValue::VersionManagerValue(const std::string& serialised_version_manager_value)
//    : structured_data_versions_(
//          [&serialised_version_manager_value](){
//            protobuf::VersionManagerValue version_manager_value_proto;
//            if (!version_manager_value_proto.ParseFromString(serialised_version_manager_value)) {
//              LOG(kError) << "Failed to read or parse serialised maid manager value.";
//             ThrowError(CommonErrors::parsing_error);
//            }
//            return StructuredDataVersions(
//                typename StructuredDataVersions::serialised_type(NonEmptyString(
//                    version_manager_value_proto.serialised_structured_data_versions())));
//          }()) {}

VersionManagerValue::VersionManagerValue(uint32_t max_versions, uint32_t max_branches)
    : structured_data_versions_(max_versions, max_branches) {}

//  Commented by Mahmoud on 5 Sep. Requires fix in structureddata
// VersionManagerValue::VersionManagerValue(const VersionManagerValue& other)
//    : structured_data_versions_(other.structured_data_versions_) {}

//  Commented by Mahmoud on 5 Sep. Requires fix in structureddata
// VersionManagerValue::VersionManagerValue(VersionManagerValue&& other)
//    : structured_data_versions_(std::move(other.structured_data_versions_)) {}

VersionManagerValue& VersionManagerValue::operator=(VersionManagerValue other) {
  swap(*this, other);
  return *this;
}

std::string VersionManagerValue::Serialise() const {
  protobuf::VersionManagerValue version_manager_value_proto;
  version_manager_value_proto.set_serialised_structured_data_versions(
      structured_data_versions_.Serialise()->string());
  return version_manager_value_proto.SerializeAsString();
}

void VersionManagerValue::Put(const StructuredDataVersions::VersionName& old_version,
                              const StructuredDataVersions::VersionName& new_version) {
  structured_data_versions_.Put(old_version, new_version);
}

std::vector<StructuredDataVersions::VersionName> VersionManagerValue::Get() const {
  return structured_data_versions_.Get();
}

std::vector<StructuredDataVersions::VersionName> VersionManagerValue::GetBranch(
    const StructuredDataVersions::VersionName& branch_tip) const {
  return structured_data_versions_.GetBranch(branch_tip);
}

void VersionManagerValue::DeleteBranchUntilFork(
    const StructuredDataVersions::VersionName& branch_tip) {
  return structured_data_versions_.DeleteBranchUntilFork(branch_tip);
}

void swap(VersionManagerValue& lhs, VersionManagerValue& rhs) {
  using std::swap;
  swap(lhs.structured_data_versions_, rhs.structured_data_versions_);
}

// bool operator==(const VersionManagerValue& lhs, const VersionManagerValue& rhs) {
//  return lhs.structured_data_versions_ == rhs.structured_data_versions_;
//}

}  // namespace vault

}  // namespace maidsafe
