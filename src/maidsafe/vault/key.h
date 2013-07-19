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
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace test {
class KeyTest_BEH_Serialise_Test;
class KeyTest_BEH_All_Test;
}  // namespace test

template<typename Persona>
class ManagerDb;

template<typename Data, int PaddedWidth>
struct Key {
  explicit Key(const typename Data::name_type& name_in);
  explicit Key(const std::string& serialised_key);
  Key(const Key& other);
  Key(Key&& other);
  Key& operator=(Key other);
  std::string Serialise() const;

  typename Data::name_type name;

  template<typename Persona>
  friend class ManagerDb;

 private:
  typedef detail::BoundedString<NodeId::kSize + PaddedWidth,
                                NodeId::kSize + PaddedWidth> FixedWidthString;

  explicit Key(const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};



template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>::Key(const typename Data::name_type& name_in) : name(name_in) {}

template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>::Key(const std::string& serialised_key)
    : name([&serialised_key]()->Identity {
        protobuf::Key key_proto;
        if (!key_proto.ParseFromString(serialised_key))
          ThrowError(CommonErrors::parsing_error);
        assert(static_cast<DataTagValue>(key_proto.type) == Data::name_type::tag_type::kEnumValue);
        return Identity(key_proto.name());
      }()) {}

template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>::Key(const FixedWidthString& fixed_width_string)
    : name([&fixed_width_string]()->Identity {
        assert(static_cast<DataTagValue>(detail::FromFixedWidthString<PaddedWidth>(
            fixed_width_string.string().substr(NodeId::kSize))) ==
                Data::name_type::tag_type::kEnumValue);
        return Identity(fixed_width_string.string().substr(0, NodeId::kSize));
      }()) {}

template<typename Data, int PaddedWidth>
void swap(Key<Data, PaddedWidth>& lhs, Key<Data, PaddedWidth>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
}

template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>::Key(const Key& other) : name(other.name) {}

template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>::Key(Key&& other) : name(std::move(other.name)) {}

template<typename Data, int PaddedWidth>
Key<Data, PaddedWidth>& Key<Data, PaddedWidth>::operator=(Key other) {
  swap(*this, other);
  return *this;
}

template<typename Data, int PaddedWidth>
std::string Key<Data, PaddedWidth>::Serialise() const {
  protobuf::Key key_proto;
  key_proto.set_name(name->string());
  key_proto.set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  return key_proto.SerializeAsString();
}

template<typename Data, int PaddedWidth>
typename Key<Data, PaddedWidth>::FixedWidthString
    Key<Data, PaddedWidth>::ToFixedWidthString() const {
  return FixedWidthString(name->string() +
                          detail::ToFixedWidthString<PaddedWidth>(
                              static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue)));
}

template<typename Data, int PaddedWidth>
bool operator==(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return lhs.name == rhs.name;
}

template<typename Data, int PaddedWidth>
bool operator!=(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename Data, int PaddedWidth>
bool operator<(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return lhs.name < rhs.name;
}

template<typename Data, int PaddedWidth>
bool operator>(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return operator<(rhs, lhs);
}

template<typename Data, int PaddedWidth>
bool operator<=(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename Data, int PaddedWidth>
bool operator>=(const Key<Data, PaddedWidth>& lhs, const Key<Data, PaddedWidth>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_H_
