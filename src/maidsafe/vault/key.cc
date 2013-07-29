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

#include "maidsafe/vault/key.h"

#include <tuple>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/key.pb.h"


namespace maidsafe {

namespace vault {

Key::Key(const Identity& name_in, DataTagValue type_in) : name(name_in), type(type_in) {}

Key::Key(const std::string& serialised_key)
    : name(),
      type(DataTagValue::kOwnerDirectoryValue) {
  protobuf::Key key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  name = Identity(key_proto.name());
  type = static_cast<DataTagValue>(key_proto.type());
}

Key::Key(const FixedWidthString& fixed_width_string)
    : name(fixed_width_string.string().substr(0, NodeId::kSize)),
      type(static_cast<DataTagValue>(
               detail::FromFixedWidthString<detail::PaddedWidth::value>(
                   fixed_width_string.string().substr(NodeId::kSize)))) {}

Key::Key(const Key& other) : name(other.name) {}

Key::Key(Key&& other) : name(std::move(other.name)) {}

Key& Key::operator=(Key other) {
  swap(*this, other);
  return *this;
}

std::string Key::Serialise() const {
  protobuf::Key key_proto;
  key_proto.set_name(name.string());
  key_proto.set_type(static_cast<int32_t>(type));
  return key_proto.SerializeAsString();
}

Key::FixedWidthString Key::ToFixedWidthString() const {
  return FixedWidthString(
      name.string() +
      detail::ToFixedWidthString<detail::PaddedWidth::value>(static_cast<uint32_t>(type)));
}

void swap(Key& lhs, Key& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.type, rhs.type);
}

bool operator==(const Key& lhs, const Key& rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type;
}

bool operator!=(const Key& lhs, const Key& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const Key& lhs, const Key& rhs) {
  return std::tie(lhs.name, lhs.type) < std::tie(rhs.name, rhs.type);
}

bool operator>(const Key& lhs, const Key& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const Key& lhs, const Key& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const Key& lhs, const Key& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
