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

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/key.pb.h"

namespace maidsafe {

namespace vault {

namespace test {
class KeyTest_BEH_Serialise_Test;
class KeyTest_BEH_All_Test;
}  // namespace test

class Db;
template<typename PersonaType>
class ManagerDb;

template <typename Persona>
struct Key {
  explicit Key(const DataNameVariant& name);
  explicit Key(const std::string& serialised_key);
  Key();
  Key(const Key& other);
  Key(Key&& other);
  Key& operator=(Key other);

  DataNameVariant name() const { return name_; }

  void swap(Key& lhs, Key& rhs) MAIDSAFE_NOEXCEPT;
  bool operator==(const Key& lhs, const Key& rhs);
  bool operator<(const Key& lhs, const Key& rhs);

  std::string ToFixedWidthString();
  std::string Serialise() const;
  DataNameVariant name;
};


template <typename Persona>
Key::Key(const DataNameVariant& name) : name_(name) {}

template <typename Persona>
Key::Key() : name_() {}

template <typename Persona>
Key::Key(const std::string& serialised_key) : name_() {
  protobuf::Key key_proto;
  key_proto.ParseFromString(serialised_key);
  name = GetDataNameVariant(static_cast<DataTagValue>(key_proto.type),
                            Identity(key_proto.name));

}
//template <typename Persona>
//Key::Key(const std::string& serialised_key) : name_() {
//  std::string name(serialised_key.substr(0, NodeId::kSize));
//  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
//  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
//  name_ = GetDataNameVariant(type, Identity(name));
//}

template <typename Persona>
Key::Key(const Key& other) : name_(other.name_)  {}

template <typename Persona>
Key::Key(Key&& other) : name_(std::move(other.name_)) {}

template <typename Persona>
Key& Key::operator=(Key other) {
  swap(*this, other);
  return *this;
}

template <typename Persona>
std::string Key::Serialise() const {
  protobuf::Key key_proto;
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name));
  key_proto.set_name(result.second.string());
  key_proto.set_type(static_cast<int32_t>(result.first));
}

template <typename Persona>
std::string Key::ToFixedWidthString() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name));
  return std::string(result.second.string() +
            detail::ToFixedWidthString<Persona::kPaddedWidth>(static_cast<uint32_t>(result.first)));
}

void swap(Key& lhs, Key& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name_, rhs.name_);
}


bool operator==(const Key& lhs, const Key& rhs) {
  return lhs.name_ == rhs.name_;
}

bool operator!=(const Key& lhs, const Key& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const Key& lhs, const Key& rhs) {
  return lhs.name_ < rhs.name_;
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

#endif  // MAIDSAFE_VAULT_KEY_H_
