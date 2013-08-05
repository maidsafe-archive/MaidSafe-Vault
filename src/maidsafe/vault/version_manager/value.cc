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

#include "maidsafe/vault/version_manager/value.h"

#include <functional>
#include <limits>
#include <utility>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"


namespace maidsafe {

namespace vault {

VersionManagerValue::VersionManagerValue(const std::string& serialised_version_manager_value)
    : structured_data_versions_(
          [&serialised_version_manager_value](){
            protobuf::VersionManagerValue version_manager_value_proto;
            if (!version_manager_value_proto.ParseFromString(serialised_maid_manager_value)) {
              LOG(kError) << "Failed to read or parse serialised maid manager value.";
             ThrowError(CommonErrors::parsing_error);
            }
            return StructuredDataVersions(
                StructuredDataVersions::serialised_type(NonEmptyString(
                    version_manager_value_proto.serialised_structured_data_versions())));
          }()) {}

VersionManagerValue::VersionManagerValue(uint32_t max_versions, uint32_t max_branches)
     : structured_data_versions(max_versions, max_branches) {}

VersionManagerValue::VersionManagerValue(const VersionManagerValue& other)
    : structured_data_versions_(other.structured_data_versions_) {}

VersionManagerValue::VersionManagerValue(VersionManagerValue&& other)
    : structured_data_versions_(std::move(other.structured_data_versions_)) {}

VersionManagerValue& VersionManagerValue::operator=(VersionManagerValue other) {
  swap(*this, other);
  return *this;
}

std::string VersionManagerValue::Serialise() const {

  protobuf::VersionManagerValue version_manager_value_proto;
  version_manager_value_proto.set_serialised_structured_data_versions(
      structured_data_versions_.Serialize);
  return version_manager_value_proto.SerializeAsString();
}

void VersionManagerValue::Put(const VersionName& old_version, const VersionName& new_version) {
  structured_data_versions_.Put(old_version, new_version);
}

std::vector<StructuredDataVersions::VersionName> VersionManagerValue::Get() const {
  return structured_data_versions_.Get();
}

std::vector<StructuredDataVersions::VersionName> VersionManagerValue::GetBranch(
    const VersionName& branch_tip) const {
  return structured_data_versions_.GetBranch(branch_tip);
}

std::vector<StructuredDataVersions::VersionName> VersionManagerValue::DeleteBranchUntilFork(
    const VersionName& branch_tip) {
  return structured_data_versions_.DeleteBranchUntilFork(branch_tip);
}

void swap(VersionManagerValue& lhs, VersionManagerValue& rhs) {
  using std::swap;
  swap(lhs.count_, rhs.count_);
  swap(lhs.total_cost_, rhs.total_cost_);
}

bool operator==(const VersionManagerValue& lhs, const VersionManagerValue& rhs) {
  return lhs.structured_data_versions_ == rhs.structured_data_versions_;
}

}  // namespace vault

}  // namespace maidsafe
