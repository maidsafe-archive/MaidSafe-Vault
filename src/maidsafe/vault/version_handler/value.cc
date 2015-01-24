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

#include "maidsafe/vault/version_handler/value.h"

#include <functional>
#include <limits>
#include <utility>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/version_handler/version_handler.pb.h"

namespace maidsafe {

namespace vault {

VersionHandlerValue::VersionHandlerValue(const std::string& serialised_version_handler_value)
    : structured_data_versions_(std::move(
          [&serialised_version_handler_value]()->StructuredDataVersionsPtr {
            protobuf::VersionHandlerValue version_handler_value_proto;
            if (!version_handler_value_proto.ParseFromString(serialised_version_handler_value)) {
              LOG(kError) << "Failed to read or parse serialised verison handler value.";
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            }
            return StructuredDataVersionsPtr(new StructuredDataVersions(
                StructuredDataVersions::serialised_type(NonEmptyString(
                    version_handler_value_proto.serialised_structured_data_versions()))));
          }())) {}

VersionHandlerValue::VersionHandlerValue(const VersionHandlerValue& other)
    : structured_data_versions_(std::move(
          [&other]()->StructuredDataVersionsPtr {
            protobuf::VersionHandlerValue version_handler_value_proto;
            if (!version_handler_value_proto.ParseFromString(other.Serialise())) {
              LOG(kError) << "Failed to read or parse serialised verison handler value.";
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            }
            return StructuredDataVersionsPtr(new StructuredDataVersions(
                StructuredDataVersions::serialised_type(NonEmptyString(
                    version_handler_value_proto.serialised_structured_data_versions()))));
          }())) {}

VersionHandlerValue::VersionHandlerValue(uint32_t max_versions, uint32_t max_branches)
    : structured_data_versions_(new StructuredDataVersions(max_versions, max_branches)) {}

VersionHandlerValue::VersionHandlerValue(VersionHandlerValue&& other) MAIDSAFE_NOEXCEPT
    : structured_data_versions_(std::move(other.structured_data_versions_)) {}

VersionHandlerValue& VersionHandlerValue::operator=(VersionHandlerValue other) {
  swap(*this, other);
  return *this;
}

std::string VersionHandlerValue::Serialise() const {
  protobuf::VersionHandlerValue version_handler_value_proto;
  version_handler_value_proto.set_serialised_structured_data_versions(
      structured_data_versions_->Serialise()->string());
  return version_handler_value_proto.SerializeAsString();
}

boost::optional<StructuredDataVersions::VersionName> VersionHandlerValue::Put(
    const StructuredDataVersions::VersionName& old_version,
    const StructuredDataVersions::VersionName& new_version) {
  return structured_data_versions_->Put(old_version, new_version);
}

std::vector<StructuredDataVersions::VersionName> VersionHandlerValue::Get() const {
  return structured_data_versions_->Get();
}

std::vector<StructuredDataVersions::VersionName> VersionHandlerValue::GetBranch(
    const StructuredDataVersions::VersionName& branch_tip) const {
  return structured_data_versions_->GetBranch(branch_tip);
}

void VersionHandlerValue::DeleteBranchUntilFork(
    const StructuredDataVersions::VersionName& branch_tip) {
  return structured_data_versions_->DeleteBranchUntilFork(branch_tip);
}

void swap(VersionHandlerValue& lhs, VersionHandlerValue& rhs) {
  using std::swap;
  swap(lhs.structured_data_versions_, rhs.structured_data_versions_);
}

bool operator==(const VersionHandlerValue& lhs, const VersionHandlerValue& rhs) {
  bool result(lhs.structured_data_versions_->max_versions() ==
                  rhs.structured_data_versions_->max_versions() &&
              lhs.structured_data_versions_->max_branches() ==
                  rhs.structured_data_versions_->max_branches());
  if (result) {
    auto lhs_versions(lhs.structured_data_versions_->Get());
    auto rhs_versions(rhs.structured_data_versions_->Get());
    result = lhs_versions.size() == rhs_versions.size();
    if (result) {
      for (size_t i(0); i< lhs_versions.size(); ++i)
        if (lhs_versions[i] != rhs_versions[0])
          return false;
    }
  }
  return result;
}

std::string VersionHandlerValue::Print() const {
  std::stringstream stream;
  stream << "\n\t[max_versions," << structured_data_versions_->max_versions()
         << "] [max_branches," << structured_data_versions_->max_branches()
         << "] [current_versions," << structured_data_versions_->Get().size() << "]";
  return stream.str();
}

VersionHandlerValue VersionHandlerValue::Resolve(const std::vector<VersionHandlerValue>& values) {
  std::vector<std::pair<VersionHandlerValue, unsigned int>> stats;
  for (const auto& value : values) {
    auto iter(std::find_if(std::begin(stats), std::end(stats),
                           [&](const std::pair<VersionHandlerValue, unsigned int>& pair) {
                             return value == pair.first;
                           }));
    if (iter == std::end(stats))
      stats.push_back(std::make_pair(value, 1));
    else
      iter->second++;
  }

  auto max_iter(std::begin(stats));
  for (auto iter(std::begin(stats)); iter != std::end(stats); ++iter)
    max_iter = (iter->second > max_iter->second) ? iter : max_iter;

  if (max_iter->second == (routing::Parameters::group_size + 1) / 2)
    return max_iter->first;

  if (values.size() == routing::Parameters::group_size - 1)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::failed_to_handle_request));

  BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
}

}  // namespace vault

}  // namespace maidsafe
