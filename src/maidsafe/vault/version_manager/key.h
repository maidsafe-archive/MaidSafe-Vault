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

#include "maidsafe/vault/key.h"
#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

class Db;
template<typename PersonaType>
class ManagerDb;

template<typename Data>
struct Key<VersionManager, Data> {
  Key(const typename Data::name_type& name_in, const Identity& originator_in);
  explicit Key(const std::string& serialised_key);
  Key(const Key& other);
  Key(Key&& other);
  Key& operator=(Key other);
  std::string Serialise() const;

  typename Data::name_type name;
  Identity originator;

  friend class Db;
  template<typename PersonaType>
  friend class ManagerDb;

 private:
  std::string ToFixedWidthString() const;
};


template<typename Data>
Key<VersionManager, Data>::Key(const typename Data::name_type& name_in,
                               const Identity& originator_in)
    : name(name_in),
      originator(originator_in) {}

template<typename Data>
Key<VersionManager, Data>::Key(const std::string& serialised_key)
    : name(),
      originator() {
  protobuf::VersionManagerKey key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  assert(static_cast<DataTagValue>(key_proto.type) == Data::name_type::tag_type::kEnumValue);
  name = Data::name_type(Identity(key_proto.name));
  originator = Identity(key_proto.originator);
}

template<typename Data>
void swap(Key<VersionManager, Data>& lhs, Key<VersionManager, Data>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.originator, rhs.originator);
}

//template<typename VersionManager, typename Data>
//Key::Key(const std::string& fixed_width_serialised_key) : name_() {
//  std::string name(serialised_key.substr(0, NodeId::kSize));
//  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
//  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
//  name_ = GetDataNameVariant(type, Identity(name));
//}

template<typename Data>
Key<VersionManager, Data>::Key(const Key& other) : name(other.name), originator(other.originator) {}

template<typename Data>
Key<VersionManager, Data>::Key(Key&& other)
    : name(std::move(other.name)),
      originator(std::move(other.originator)) {}

template<typename Data>
Key<VersionManager, Data>& Key<VersionManager, Data>::operator=(Key other) {
  swap(*this, other);
  return *this;
}

template<typename Data>
std::string Key<VersionManager, Data>::Serialise() const {
  protobuf::Key key_proto;
  key_proto.set_name(name->string());
  key_proto.set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  key_proto.set_originator(originator.string());
  return key_proto.SerializeAsString();
}

template<typename Data>
std::string Key<VersionManager, Data>::ToFixedWidthString() const {
  return name->string() + detail::ToFixedWidthString<VersionManager::kPaddedWidth>(
      static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue)) +
      originator.string();
}

template<typename Data>
bool operator==(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return lhs.name == rhs.name && lhs.originator == rhs.originator;
}

template<typename Data>
bool operator!=(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename Data>
bool operator<(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return std::tie(lhs.name, lhs.originator) < std::tie(rhs.name, rhs.originator);
}

template<typename Data>
bool operator>(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return operator<(rhs, lhs);
}

template<typename Data>
bool operator<=(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename Data>
bool operator>=(const Key<VersionManager, Data>& lhs, const Key<VersionManager, Data>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_KEY_H_
