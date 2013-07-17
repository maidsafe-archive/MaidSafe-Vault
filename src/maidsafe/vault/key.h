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

#ifndef MAIDSAFE_VAULT_KEY_H_
#define MAIDSAFE_VAULT_KEY_H_

#include <string>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/key.pb.h"


namespace maidsafe {

namespace vault {

namespace test {
class KeyTest_BEH_Serialise_Test;
class KeyTest_BEH_All_Test;
}  // namespace test

class Db;
template<typename Persona>
class ManagerDb;

template<typename Data>
struct Key {
  explicit Key(const typename Data::name_type& name_in);
  explicit Key(const std::string& serialised_key);
  Key(const Key& other);
  Key(Key&& other);
  Key& operator=(Key other);
  std::string Serialise() const;

  typename Data::name_type name;

  friend class Db;
  template<typename Persona>
  friend class ManagerDb;

 private:
  template<typename Persona>
  std::string ToFixedWidthString() const;
};


template<typename Data>
Key<Data>::Key(const typename Data::name_type& name_in) : name(name_in) {}

template<typename Data>
Key<Data>::Key(const std::string& serialised_key)
    : name([&serialised_key]()->Identity {
        protobuf::Key key_proto;
        if (!key_proto.ParseFromString(serialised_key))
          ThrowError(CommonErrors::parsing_error);
        assert(static_cast<DataTagValue>(key_proto.type) == Data::name_type::tag_type::kEnumValue);
        return Identity(key_proto.name);
      }()) {}

template<typename Data>
void swap(Key<Data>& lhs, Key<Data>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
}

//template<typename Persona, typename Data>
//Key::Key(const std::string& fixed_width_serialised_key) : name_() {
//  std::string name(serialised_key.substr(0, NodeId::kSize));
//  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
//  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
//  name_ = GetDataNameVariant(type, Identity(name));
//}

template<typename Data>
Key<Data>::Key(const Key& other) : name(other.name) {}

template<typename Data>
Key<Data>::Key(Key&& other) : name(std::move(other.name)) {}

template<typename Data>
Key<Data>& Key<Data>::operator=(Key other) {
  swap(*this, other);
  return *this;
}

template<typename Data>
std::string Key<Data>::Serialise() const {
  protobuf::Key key_proto;
  key_proto.set_name(name->string());
  key_proto.set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  return key_proto.SerializeAsString();
}

template<typename Data>
template<typename Persona>
std::string Key<Data>::ToFixedWidthString() const {
  return name->string() + detail::ToFixedWidthString<Persona::kPaddedWidth>(
      static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue));
}

template<typename Data>
bool operator==(const Key<Data>& lhs, const Key<Data>& rhs) {
  return lhs.name == rhs.name;
}

template<typename Data>
bool operator!=(const Key<Data>& lhs, const Key<Data>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename Data>
bool operator<(const Key<Data>& lhs, const Key<Data>& rhs) {
  return lhs.name < rhs.name;
}

template<typename Data>
bool operator>(const Key<Data>& lhs, const Key<Data>& rhs) {
  return operator<(rhs, lhs);
}

template<typename Data>
bool operator<=(const Key<Data>& lhs, const Key<Data>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename Data>
bool operator>=(const Key<Data>& lhs, const Key<Data>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_H_
