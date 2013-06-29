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

#include "maidsafe/vault/version_manager/version_manager_key.h"

#include <algorithm>
#include <tuple>

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

const int VersionManagerKey::kPaddedWidth_(1);

VersionManagerKey::VersionManagerKey() : data_name_(), originator_() {}

VersionManagerKey::VersionManagerKey(const DataNameVariant& data_name,
                                     const Identity& originator)
    : data_name_(data_name),
      originator_(originator) {}

VersionManagerKey::VersionManagerKey(const std::string& serialised_key)
    : data_name_(),
      originator_() {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
  data_name_ = GetDataNameVariant(type, Identity(name));
  originator_ = Identity(serialised_key.substr(NodeId::kSize + kPaddedWidth_));
}

VersionManagerKey::VersionManagerKey(const VersionManagerKey& other)
    : data_name_(other.data_name_),
      originator_(other.originator_) {}

VersionManagerKey& VersionManagerKey::operator=(VersionManagerKey other) {
  swap(*this, other);
  return *this;
}

VersionManagerKey::VersionManagerKey(VersionManagerKey&& other)
    : data_name_(std::move(other.data_name_)),
      originator_(std::move(other.originator_)) {}

void swap(VersionManagerKey& lhs, VersionManagerKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name_, rhs.data_name_);
  swap(lhs.originator_, rhs.originator_);
}

std::string VersionManagerKey::Serialise() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, data_name_));
  return std::string(
      result.second.string() +
      detail::ToFixedWidthString<kPaddedWidth_>(static_cast<uint32_t>(result.first)) +
      originator_.string());
}

bool operator==(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return std::tie(lhs.data_name_, lhs.originator_) == std::tie(rhs.data_name_, rhs.originator_);
}

bool operator!=(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return std::tie(lhs.data_name_, lhs.originator_) < std::tie(rhs.data_name_, rhs.originator_);
}

bool operator>(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
