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

#include "maidsafe/vault/data_manager/key.h"

#include <tuple>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/data_manager/key.pb.h"

namespace maidsafe {

namespace vault {

DataManagerKey::DataManagerKey()
    : name(), type(), originator() {}

DataManagerKey::DataManagerKey(const std::string& serialised_key)
    : name(), type(DataTagValue::kOwnerDirectoryValue), originator() {
  protobuf::DataManagerKey key_proto;
  if (!key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  name = Identity(key_proto.name());
  type = static_cast<DataTagValue>(key_proto.type());
  originator = Identity(key_proto.originator());
}

DataManagerKey::DataManagerKey(const Identity& name_in, const DataTagValue type_in,
                               const Identity& originator_in)
    : name(name_in), type(type_in), originator(originator_in) {}

DataManagerKey::DataManagerKey(const DataManagerKey& other)
    : name(other.name), type(other.type), originator(other.originator) {}

DataManagerKey::DataManagerKey(DataManagerKey&& other)
    : name(std::move(other.name)),
      type(std::move(other.type)),
      originator(std::move(other.originator)) {}

DataManagerKey& DataManagerKey::operator=(DataManagerKey other) {
  swap(*this, other);
  return *this;
}

std::string DataManagerKey::Serialise() const {
  protobuf::DataManagerKey key_proto;
  key_proto.set_name(name.string());
  key_proto.set_type(static_cast<int32_t>(type));
  key_proto.set_originator(originator.string());
  return key_proto.SerializeAsString();
}

DataManagerKey::FixedWidthString DataManagerKey::ToFixedWidthString() const {
  return FixedWidthString(name.string() + detail::ToFixedWidthString<detail::PaddedWidth::value>(
                                              static_cast<uint32_t>(type)) +
                          originator.string());
}

void swap(DataManagerKey& lhs, DataManagerKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.type, rhs.type);
  swap(lhs.originator, rhs.originator);
}

bool operator==(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return lhs.name == rhs.name && lhs.type == rhs.type && lhs.originator == rhs.originator;
}

bool operator!=(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return std::tie(lhs.name, lhs.type, lhs.originator) <
         std::tie(rhs.name, rhs.type, rhs.originator);
}

bool operator>(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const DataManagerKey& lhs, const DataManagerKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
