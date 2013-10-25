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

#include "maidsafe/vault/version_handler/key.h"

#include <tuple>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/version_handler/key.pb.h"

namespace maidsafe {

namespace vault {

VersionHandlerKey::VersionHandlerKey(const std::string& serialised_key)
    : name(), type(DataTagValue::kOwnerDirectoryValue), originator() {
  protobuf::VersionHandlerKey key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  name = Identity(key_proto.name());
  type = static_cast<DataTagValue>(key_proto.type());
  originator = Identity(key_proto.originator());
}

VersionHandlerKey::VersionHandlerKey(const Identity& name_in, const DataTagValue type_in,
                                     const Identity& originator_in)
    : name(name_in), type(type_in), originator(originator_in) {}

VersionHandlerKey::VersionHandlerKey(const FixedWidthString& fixed_width_string)
    : name(fixed_width_string.string().substr(0, NodeId::kSize)),
      type(static_cast<DataTagValue>(detail::FromFixedWidthString<detail::PaddedWidth::value>(
          fixed_width_string.string().substr(NodeId::kSize, detail::PaddedWidth::value)))),
      originator(fixed_width_string.string().substr(NodeId::kSize + detail::PaddedWidth::value)) {}

VersionHandlerKey::VersionHandlerKey(const VersionHandlerKey& other)
    : name(other.name), type(other.type), originator(other.originator) {}

VersionHandlerKey::VersionHandlerKey(VersionHandlerKey&& other)
    : name(std::move(other.name)),
      type(std::move(other.type)),
      originator(std::move(other.originator)) {}

VersionHandlerKey& VersionHandlerKey::operator=(VersionHandlerKey other) {
  swap(*this, other);
  return *this;
}

std::string VersionHandlerKey::Serialise() const {
  protobuf::VersionHandlerKey key_proto;
  key_proto.set_name(name.string());
  key_proto.set_type(static_cast<int32_t>(type));
  key_proto.set_originator(originator.string());
  return key_proto.SerializeAsString();
}

VersionHandlerKey::FixedWidthString VersionHandlerKey::ToFixedWidthString() const {
  return FixedWidthString(name.string() + detail::ToFixedWidthString<detail::PaddedWidth::value>(
                                              static_cast<uint32_t>(type)) +
                          originator.string());
}

void swap(VersionHandlerKey& lhs, VersionHandlerKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.type, rhs.type);
  swap(lhs.originator, rhs.originator);
}

bool operator==(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type && lhs.originator == rhs.originator;
}

bool operator!=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return std::tie(lhs.name, lhs.type, lhs.originator) <
         std::tie(rhs.name, rhs.type, rhs.originator);
}

bool operator>(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const VersionHandlerKey& lhs, const VersionHandlerKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
