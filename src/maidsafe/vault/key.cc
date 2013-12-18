/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/key.h"

#include <tuple>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/key.pb.h"

namespace maidsafe {

namespace vault {

Key::Key() : name(), type() {}

Key::Key(const Identity& name_in, DataTagValue type_in) : name(name_in), type(type_in) {}

Key::Key(const std::string& serialised_key) : name(), type(DataTagValue::kMutableDataValue) {
  protobuf::Key key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  name = Identity(key_proto.name());
  type = static_cast<DataTagValue>(key_proto.type());
}

Key::Key(const FixedWidthString& fixed_width_string)
    : name(fixed_width_string.string().substr(0, NodeId::kSize)),
      type(static_cast<DataTagValue>(detail::FromFixedWidthString<detail::PaddedWidth::value>(
          fixed_width_string.string().substr(NodeId::kSize)))) {}

Key::Key(const Key& other) : name(other.name), type(other.type) {}

Key::Key(Key&& other) : name(std::move(other.name)), type(std::move(other.type)) {}

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
  return FixedWidthString(name.string() + detail::ToFixedWidthString<detail::PaddedWidth::value>(
                                              static_cast<uint32_t>(type)));
}

void swap(Key& lhs, Key& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.type, rhs.type);
}

bool operator==(const Key& lhs, const Key& rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type;
}

bool operator!=(const Key& lhs, const Key& rhs) { return !operator==(lhs, rhs); }

bool operator<(const Key& lhs, const Key& rhs) {
  return std::tie(lhs.name, lhs.type) < std::tie(rhs.name, rhs.type);
}

bool operator>(const Key& lhs, const Key& rhs) { return operator<(rhs, lhs); }

bool operator<=(const Key& lhs, const Key& rhs) { return !operator>(lhs, rhs); }

bool operator>=(const Key& lhs, const Key& rhs) { return !operator<(lhs, rhs); }

}  // namespace vault

}  // namespace maidsafe
