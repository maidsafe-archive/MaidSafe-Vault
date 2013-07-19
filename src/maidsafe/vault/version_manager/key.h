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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_

#include <tuple>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/node_id.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/version_manager/key.pb.h"
#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

template<typename Data>
struct VersionManagerKey {
  VersionManagerKey(const typename Data::name_type& name_in, const Identity& originator_in);
  explicit VersionManagerKey(const std::string& serialised_key);
  VersionManagerKey(const VersionManagerKey& other);
  VersionManagerKey(VersionManagerKey&& other);
  VersionManagerKey& operator=(VersionManagerKey other);
  std::string Serialise() const;

  typename Data::name_type name;
  Identity originator;

 private:
  static const int kPaddedWidth = 1;
  typedef detail::BoundedString<NodeId::kSize + kPaddedWidth,
                                NodeId::kSize + kPaddedWidth> FixedWidthString;

  explicit VersionManagerKey(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};



template<typename Data>
VersionManagerKey<Data>::VersionManagerKey(const typename Data::name_type& name_in,
                                           const Identity& originator_in)
    : name(name_in),
      originator(originator_in) {}

template<typename Data>
VersionManagerKey<Data>::VersionManagerKey(const std::string& serialised_key)
    : name(),
      originator() {
  protobuf::VersionManagerKey key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  assert(static_cast<DataTagValue>(key_proto.type()) == Data::name_type::tag_type::kEnumValue);
  name = Data::name_type(Identity(key_proto.name()));
  originator = Identity(key_proto.originator());
}

template<typename Data>
VersionManagerKey<Data>::VersionManagerKey(const FixedWidthString& fixed_width_string)
    : name([&fixed_width_string]()->Identity {
        assert(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth>(
            fixed_width_string.string().substr(NodeId::kSize, kPaddedWidth))) ==
                Data::name_type::tag_type::kEnumValue);
        return Identity(fixed_width_string.string().substr(0, NodeId::kSize));
      }()),
      originator([&fixed_width_string]()->std::string {
        return fixed_width_string.string().substr(NodeId::kSize + kPaddedWidth);
      }()) {}

template<typename Data>
void swap(VersionManagerKey<Data>& lhs, VersionManagerKey<Data>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.originator, rhs.originator);
}

template<typename Data>
VersionManagerKey<Data>::VersionManagerKey(const VersionManagerKey& other)
    : name(other.name),
      originator(other.originator) {}

template<typename Data>
VersionManagerKey<Data>::VersionManagerKey(VersionManagerKey&& other)
    : name(std::move(other.name)),
      originator(std::move(other.originator)) {}

template<typename Data>
VersionManagerKey<Data>& VersionManagerKey<Data>::operator=(VersionManagerKey other) {
  swap(*this, other);
  return *this;
}

template<typename Data>
std::string VersionManagerKey<Data>::Serialise() const {
  protobuf::VersionManagerKey key_proto;
  key_proto.set_name(name->string());
  key_proto.set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  key_proto.set_originator(originator.string());
  return key_proto.SerializeAsString();
}

template<typename Data>
typename VersionManagerKey<Data>::FixedWidthString
    VersionManagerKey<Data>::ToFixedWidthString() const {
  return FixedWidthString(name->string() +
                          detail::ToFixedWidthString<kPaddedWidth>(
                              static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue)) +
                          originator.string());
}

template<typename Data>
bool operator==(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return lhs.name == rhs.name && lhs.originator == rhs.originator;
}

template<typename Data>
bool operator!=(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename Data>
bool operator<(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return std::tie(lhs.name, lhs.originator) < std::tie(rhs.name, rhs.originator);
}

template<typename Data>
bool operator>(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return operator<(rhs, lhs);
}

template<typename Data>
bool operator<=(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename Data>
bool operator>=(const VersionManagerKey<Data>& lhs, const VersionManagerKey<Data>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
