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

#include "maidsafe/vault/version_manager/key.h"
#include "maidsafe/vault/version_manager/key.pb.h"


namespace maidsafe {

namespace vault {

VersionManagerKey::VersionManagerKey(const std::string& serialised_key)
    : data_name(),
      data_type(DataTagValue::kOwnerDirectoryValue),
      originator() {
  protobuf::VersionManagerKey key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  assert(static_cast<DataTagValue>(key_proto.type()) == Data::name_type::tag_type::kEnumValue);
  name = Data::name_type(Identity(key_proto.name()));
  originator = Identity(key_proto.originator());
}

VersionManagerKey::VersionManagerKey(const FixedWidthString& fixed_width_string)
    : name([&fixed_width_string]()->Identity {
        assert(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth>(
            fixed_width_string.string().substr(NodeId::kSize, kPaddedWidth))) ==
                Data::name_type::tag_type::kEnumValue);
        return Identity(fixed_width_string.string().substr(0, NodeId::kSize));
      }()),
      originator([&fixed_width_string]()->std::string {
        return fixed_width_string.string().substr(NodeId::kSize + kPaddedWidth);
      }()) {}

VersionManagerKey::VersionManagerKey(const VersionManagerKey& other)
    : name(other.name),
      originator(other.originator) {}

VersionManagerKey::VersionManagerKey(VersionManagerKey&& other)
    : name(std::move(other.name)),
      originator(std::move(other.originator)) {}

VersionManagerKey& VersionManagerKey::operator=(VersionManagerKey other) {
  swap(*this, other);
  return *this;
}

std::string VersionManagerKey::Serialise() const {
  protobuf::VersionManagerKey key_proto;
  key_proto.set_name(name->string());
  key_proto.set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  key_proto.set_originator(originator.string());
  return key_proto.SerializeAsString();
}

typename VersionManagerKey::FixedWidthString
    VersionManagerKey::ToFixedWidthString() const {
  return FixedWidthString(name->string() +
                          detail::ToFixedWidthString<kPaddedWidth>(
                              static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue)) +
                          originator.string());
}






void swap(VersionManagerKey& lhs, VersionManagerKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name, rhs.data_name);
  swap(lhs.data_type, rhs.data_type);
  swap(lhs.originator, rhs.originator);
}

bool operator==(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return lhs.data_name == rhs.data_name &&
         lhs.data_type == rhs.data_type &&
         lhs.originator == rhs.originator;
}

bool operator!=(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const VersionManagerKey& lhs, const VersionManagerKey& rhs) {
  return std::tie(lhs.data_name, lhs.data_type, lhs.originator) <
         std::tie(rhs.data_name, rhs.data_type, rhs.originator);
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

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
