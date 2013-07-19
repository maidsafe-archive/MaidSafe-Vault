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

#include "maidsafe/common/error.h"

#include "maidsafe/vault/group_key.pb.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

class Db;

template<typename GroupName, typename Data, int PaddedWidth>
struct GroupKey {
  GroupKey(const typename GroupName& group_name_in, const typename Data::name_type& data_name_in);
  explicit GroupKey(const std::string& serialised_group_key);
  GroupKey(const GroupKey& other);
  GroupKey(GroupKey&& other);
  GroupKey& operator=(GroupKey other);
  std::string Serialise() const;

  typename GroupName group_name;
  typename Data::name_type data_name;

  friend class Db;

 private:
  typedef uint32_t GroupId;
  typedef detail::BoundedString<sizeof(GroupId) + NodeId::kSize + PaddedWidth,
                                sizeof(GroupId) + NodeId::kSize + PaddedWidth> FixedWidthString;

  GroupKey(const FixedWidthString& fixed_width_string, const GroupName& group_name_in);
  FixedWidthString ToFixedWidthString(GroupId group_id) const;
  static GroupId GetGroupId(const FixedWidthString& fixed_width_string);
};



template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>::GroupKey(const typename GroupName& group_name_in,
                                                 const typename Data::name_type& data_name_in)
    : group_name(group_name_in),
      data_name(data_name_in) {}

template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>::GroupKey(const std::string& serialised_group_key)
    : group_name(group_name_in),
      data_name(data_name_in) {
  protobuf::GroupKey group_key_proto;
  if (!group_key_proto.ParseFromString(serialised_group_key))
    ThrowError(CommonErrors::parsing_error);
  assert(static_cast<DataTagValue>(group_key_proto.data_type()) ==
         Data::name_type::tag_type::kEnumValue);
  group_name = GroupName(Identity(group_key_proto.group_name()));
  data_name = Data::name_type(Identity(group_key_proto.data_name()));
}

template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>::GroupKey(const FixedWidthString& fixed_width_string,
                                                 const GroupName& group_name_in)
    : group_name(group_name_in),
      name([&fixed_width_string]()->Identity {
        assert(static_cast<DataTagValue>(detail::FromFixedWidthString<PaddedWidth>(
            fixed_width_string.string().substr(sizeof(GroupId) + NodeId::kSize))) ==
                Data::name_type::tag_type::kEnumValue);
        return Identity(fixed_width_string.string().substr(sizeof(GroupId), NodeId::kSize));
      }()) {}

template<typename GroupName, typename Data, int PaddedWidth>
void swap(GroupKey<GroupName, Data, PaddedWidth>& lhs,
          GroupKey<GroupName, Data, PaddedWidth>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.group_name, rhs.group_name);
  swap(lhs.data_name, rhs.data_name);
}

template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>::GroupKey(const GroupKey& other)
    : group_name(other.group_name),
      data_name(other.data_name) {}

template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>::GroupKey(GroupKey&& other)
    : group_name(std::move(other.group_name)),
      data_name(std::move(other.data_name)) {}

template<typename GroupName, typename Data, int PaddedWidth>
GroupKey<GroupName, Data, PaddedWidth>& GroupKey<GroupName, Data, PaddedWidth>::operator=(
    GroupKey other) {
  swap(*this, other);
  return *this;
}

template<typename GroupName, typename Data, int PaddedWidth>
std::string GroupKey<GroupName, Data, PaddedWidth>::Serialise() const {
  protobuf::GroupKey group_key_proto;
  group_key_proto.set_group_name(group_name->string());
  group_key_proto.set_data_name(data_name->string());
  group_key_proto.set_data_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  return group_key_proto.SerializeAsString();
}

template<typename GroupName, typename Data, int PaddedWidth>
typename GroupKey<GroupName, Data, PaddedWidth>::FixedWidthString
    GroupKey<GroupName, Data, PaddedWidth>::ToFixedWidthString(GroupId group_id) const {
  return FixedWidthString(detail::ToFixedWidthString<sizeof(GroupId)>(group_id) +
                          data_name->string() +
                          detail::ToFixedWidthString<PaddedWidth>(
                              static_cast<uint32_t>(Data::name_type::tag_type::kEnumValue)));
}

template<typename GroupName, typename Data, int PaddedWidth>
typename GroupKey<GroupName, Data, PaddedWidth>::GroupId
    GroupKey<GroupName, Data, PaddedWidth>::GetGroupId(const FixedWidthString& fixed_width_string) {
  return detail::FromFixedWidthString<sizeof(GroupId)>(
      fixed_width_string.substr(0, sizeof(GroupId)));
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator==(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
                const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return lhs.group_name == rhs.group_name && lhs.data_name == rhs.data_name;
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator!=(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
                const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return !operator==(lhs, rhs);
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator<(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
               const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return std::tie(lhs.group_name, lhs.data_name) < std::tie(rhs.group_name, rhs.data_name);
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator>(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
               const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return operator<(rhs, lhs);
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator<=(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
                const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return !operator>(lhs, rhs);
}

template<typename GroupName, typename Data, int PaddedWidth>
bool operator>=(const GroupKey<GroupName, Data, PaddedWidth>& lhs,
                const GroupKey<GroupName, Data, PaddedWidth>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_KEY_H_
