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

#ifndef MAIDSAFE_VAULT_GROUP_KEY_H_
#define MAIDSAFE_VAULT_GROUP_KEY_H_

#include <string>
#include <tuple>

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/node_id.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/vault/metadata_key.h"
#include "maidsafe/vault/group_key.pb.h"
#include "maidsafe/vault/key_utils.h"

namespace maidsafe {

namespace vault {

template <typename Persona>
class GroupDb;

template <typename GroupName>
struct GroupKey {
  GroupKey();
  GroupKey(const GroupName& group_name_in, const Identity& name_in, DataTagValue type_in);
  explicit GroupKey(const std::string& serialised_group_key);
  GroupKey(const GroupKey& other);
  GroupKey(GroupKey&& other);
  GroupKey& operator=(GroupKey other);
  std::string Serialise() const;

  GroupName group_name() const { return metadata_key.group_name(); }

  MetadataKey<GroupName> metadata_key;
  Identity name;
  DataTagValue type;

  template <typename Persona>
  friend class GroupDb;

 private:
  typedef maidsafe::detail::BoundedString<NodeId::kSize + detail::PaddedWidth::value,
                                          NodeId::kSize + detail::PaddedWidth::value>
      FixedWidthString;

  GroupKey(const GroupName& group_name_in, const FixedWidthString& fixed_width_string);
  FixedWidthString ToFixedWidthString() const;
};

template <typename GroupName>
GroupKey<GroupName>::GroupKey()
    : metadata_key(), name(), type() {}

template <typename GroupName>
GroupKey<GroupName>::GroupKey(const GroupName& group_name_in, const Identity& name_in,
                              DataTagValue type_in)
    : metadata_key(group_name_in), name(name_in), type(type_in) {}

template <typename GroupName>
GroupKey<GroupName>::GroupKey(const std::string& serialised_group_key)
    : metadata_key(), name(), type(DataTagValue::kMutableDataValue) {
  protobuf::GroupKey group_key_proto;
  if (!group_key_proto.ParseFromString(serialised_group_key)) {
    LOG(kError) << "GroupKey parsing error";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  metadata_key = MetadataKey<GroupName>(GroupName(Identity(group_key_proto.group_name())));
  name = Identity(group_key_proto.name());
  type = static_cast<DataTagValue>(group_key_proto.type());
}

template <typename GroupName>
GroupKey<GroupName>::GroupKey(const GroupName& group_name_in,
                              const FixedWidthString& fixed_width_string)
    : metadata_key(group_name_in),
      name(fixed_width_string.string().substr(0, NodeId::kSize)),
      type(static_cast<DataTagValue>(detail::FromFixedWidthString<detail::PaddedWidth::value>(
          fixed_width_string.string().substr(NodeId::kSize)))) {}

template <typename GroupName>
GroupKey<GroupName>::GroupKey(const GroupKey& other)
    : metadata_key(other.metadata_key), name(other.name), type(other.type) {}

template <typename GroupName>
GroupKey<GroupName>::GroupKey(GroupKey&& other)
    : metadata_key(std::move(other.metadata_key)),
      name(std::move(other.name)),
      type(std::move(other.type)) {}

template <typename GroupName>
GroupKey<GroupName>& GroupKey<GroupName>::operator=(GroupKey other) {
  swap(*this, other);
  return *this;
}

template <typename GroupName>
std::string GroupKey<GroupName>::Serialise() const {
  protobuf::GroupKey group_key_proto;
  group_key_proto.set_group_name(metadata_key.group_name()->string());
  group_key_proto.set_name(name.string());
  group_key_proto.set_type(static_cast<int32_t>(type));
  return group_key_proto.SerializeAsString();
}

template <typename GroupName>
typename GroupKey<GroupName>::FixedWidthString GroupKey<GroupName>::ToFixedWidthString() const {
  return FixedWidthString(name.string() + detail::ToFixedWidthString<detail::PaddedWidth::value>(
                                              static_cast<uint32_t>(type)));
}

template <typename GroupName>
void swap(GroupKey<GroupName>& lhs, GroupKey<GroupName>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.metadata_key, rhs.metadata_key);
  swap(lhs.name, rhs.name);
  swap(lhs.type, rhs.type);
}

template <typename GroupName>
bool operator==(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return lhs.metadata_key == rhs.metadata_key && lhs.name == rhs.name && lhs.type == rhs.type;
}

template <typename GroupName>
bool operator!=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator==(lhs, rhs);
}

template <typename GroupName>
bool operator<(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return std::tie(lhs.metadata_key, lhs.name, lhs.type) <
         std::tie(rhs.metadata_key, rhs.name, rhs.type);
}

template <typename GroupName>
bool operator>(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return operator<(rhs, lhs);
}

template <typename GroupName>
bool operator<=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator>(lhs, rhs);
}

template <typename GroupName>
bool operator>=(const GroupKey<GroupName>& lhs, const GroupKey<GroupName>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_KEY_H_
