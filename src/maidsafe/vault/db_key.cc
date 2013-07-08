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

#include "maidsafe/vault/db_key.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

const int DbKey::kPaddedWidth_(1);

DbKey::DbKey(const DataNameVariant& name) : name_(name) {}

DbKey::DbKey() : name_() {}

DbKey::DbKey(const std::string& serialised_key) : name_() {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
  name_ = GetDataNameVariant(type, Identity(name));
}

DbKey::DbKey(const DbKey& other) : name_(other.name_)  {}

DbKey::DbKey(DbKey&& other) : name_(std::move(other.name_)) {}

DbKey& DbKey::operator=(DbKey other) {
  swap(*this, other);
  return *this;
}

std::string DbKey::Serialise() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name_));
  return std::string(result.second.string() +
            detail::ToFixedWidthString<kPaddedWidth_>(static_cast<uint32_t>(result.first)));
}


void swap(DbKey& lhs, DbKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name_, rhs.name_);
}

bool operator==(const DbKey& lhs, const DbKey& rhs) {
  return lhs.name_ == rhs.name_;
}

bool operator!=(const DbKey& lhs, const DbKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const DbKey& lhs, const DbKey& rhs) {
  return lhs.name_ < rhs.name_;
}

bool operator>(const DbKey& lhs, const DbKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const DbKey& lhs, const DbKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const DbKey& lhs, const DbKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
