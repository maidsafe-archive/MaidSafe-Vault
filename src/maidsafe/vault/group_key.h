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

#ifndef MAIDSAFE_VAULT_GROUP_KEY_H_
#define MAIDSAFE_VAULT_GROUP_KEY_H_

#include <string>
#include <tuple>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/group_key.pb.h"
#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

class Db;

template<typename GroupName>
struct GroupKey {
  typedef GroupName GroupNameType;
  template<typename Data>
  GroupKey(const GroupName& group_name_in, const typename Data::name_type& data_name_in);
  explicit GroupKey(const std::string& serialised_group_key);
  GroupKey(const GroupKey& other);
  GroupKey(GroupKey&& other);
  GroupKey& operator=(GroupKey other);
  std::string Serialise() const;

  GroupName group_name;
  Identity data_name;
  DataTagValue data_type;

  friend class Db;

 private:
  typedef maidsafe::detail::BoundedString<
      NodeId::kSize + detail::PaddedWidth::value,
      NodeId::kSize + detail::PaddedWidth::value> FixedWidthString;

  GroupKey(const GroupName& group_name_in, const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};



template<typename GroupName>
template<typename Data>
GroupKey<GroupName>::GroupKey(const GroupName& group_name_in,
                              const typename Data::name_type& data_name_in)
    : group_name(group_name_in),
      data_name(data_name_in.data),
      data_type(Data::type_enum_value()) {}

template<typename GroupName>
GroupKey<GroupName>::GroupKey(const std::string& serialised_group_key)
    : group_name(),
      data_name(),
      data_type(DataTagValue::kOwnerDirectoryValue) {
  protobuf::GroupKey group_key_proto;
  if (!group_key_proto.ParseFromString(serialised_group_key))
    ThrowError(CommonErrors::parsing_error);
  group_name = GroupName(Identity(group_key_proto.group_name()));
  data_name = Identity(group_key_proto.data_name());
  data_type = static_cast<DataTagValue>(group_key_proto.data_type());
}

template<typename GroupName>
GroupKey<GroupName>::GroupKey(const GroupName& group_name_in,
                              const FixedWidthString& fixed_width_string)
    : group_name(group_name_in),
      data_name(fixed_width_string.string().substr(0, NodeId::kSize)),
      data_type(static_cast<DataTagValue>(
                    detail::FromFixedWidthString<detail::PaddedWidth::value>(
                        fixed_width_string.string().substr(NodeId::kSize)))) {}

template<typename GroupName>
GroupKey<GroupName>::GroupKey(const GroupKey& other)
    : group_name(other.group_name),
      data_name(other.data_name),
      data_type(other.data_type) {}

template<typename GroupName>
GroupKey<GroupName>::GroupKey(GroupKey&& other)
    : group_name(std::move(other.group_name)),
      data_name(std::move(other.data_name)),
      data_type(std::move(other.data_type)) {}

template<typename GroupName>
GroupKey<GroupName>& GroupKey<GroupName>::operator=(GroupKey other) {
  swap(*this, other);
  return *this;
}

template<typename GroupName>
std::string GroupKey<GroupName>::Serialise() const {
  protobuf::GroupKey group_key_proto;
  group_key_proto.set_group_name(group_name->string());
  group_key_proto.set_data_name(data_name.string());
  group_key_proto.set_data_type(static_cast<int32_t>(data_type));
  return group_key_proto.SerializeAsString();
}

template<typename GroupName>
typename GroupKey<GroupName>::FixedWidthString
    GroupKey<GroupName>::ToFixedWidthString() const {
  return FixedWidthString(
      data_name.string() +
      detail::ToFixedWidthString<detail::PaddedWidth::value>(static_cast<uint32_t>(data_type)));
}

template<typename GroupName>
void swap(GroupKey<GroupName>& lhs, GroupKey<GroupName>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.group_name, rhs.group_name);
  swap(lhs.data_name, rhs.data_name);
  swap(lhs.data_type, rhs.data_type);
}

template<typename GroupName>
bool operator==(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return lhs.group_name == rhs.group_name &&
         lhs.data_name == rhs.data_name &&
         lhs.data_type == rhs.data_type;
}

template<typename GroupName>
bool operator!=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename GroupName>
bool operator<(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return std::tie(lhs.group_name, lhs.data_name, lhs.data_type) <
         std::tie(rhs.group_name, rhs.data_name, rhs.data_type);
}

template<typename GroupName>
bool operator>(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return operator<(rhs, lhs);
}

template<typename GroupName>
bool operator<=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename GroupName>
bool operator>=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_KEY_H_
