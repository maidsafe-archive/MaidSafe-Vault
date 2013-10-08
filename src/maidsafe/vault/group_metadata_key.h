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

#ifndef MAIDSAFE_VAULT_GROUP_METADATA_KEY_H_
#define MAIDSAFE_VAULT_GROUP_METADATA_KEY_H_

#include <string>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/group_metadata_key.pb.h"

namespace maidsafe {

namespace vault {

template <typename GroupMetadataName>
struct GroupMetadataKey {
  GroupMetadataKey();  // FIXME Prakash
  GroupMetadataKey(const GroupMetadataName& group_metadata_name_in);
  explicit GroupMetadataKey(const std::string& serialised_key);
  GroupMetadataKey(const GroupMetadataKey& other);
  GroupMetadataKey(GroupMetadataKey&& other);
  GroupMetadataKey& operator=(GroupMetadataKey other);
  std::string Serialise() const;

  friend bool operator==(const GroupMetadataKey<GroupMetadataName>& lhs,
                         const GroupMetadataKey<GroupMetadataName>& rhs) {
    return (lhs.group_name == rhs.group_name);
  }

  GroupMetadataName group_name;
};


// Implementation

template <typename GroupMetadataName>
GroupMetadataKey<GroupMetadataName>::GroupMetadataKey() : group_name() {}

template <typename GroupMetadataName>
GroupMetadataKey<GroupMetadataName>::GroupMetadataKey(const GroupMetadataKey& other)
    : group_name(other.group_name) {}

template <typename GroupMetadataName>
GroupMetadataKey<GroupMetadataName>::GroupMetadataKey(const GroupMetadataName& group_name_in)
    : group_name(group_name_in) {}

template <typename GroupMetadataName>
GroupMetadataKey<GroupMetadataName>::GroupMetadataKey(const std::string& serialised_key)
    : group_name() {
  protobuf::GroupMetadataKey group_metadata_key_proto;
  if (!group_metadata_key_proto.ParseFromString(serialised_key))
    ThrowError(CommonErrors::parsing_error);
  group_name = GroupMetadataName(Identity(group_metadata_key_proto.group_name()));
}

template <typename GroupMetadataName>
std::string GroupMetadataKey<GroupMetadataName>::Serialise() const {
  protobuf::GroupMetadataKey group_metadata_key_proto;
  group_metadata_key_proto.set_group_name(group_name->string());
  return group_metadata_key_proto.SerializeAsString();
}

template <typename GroupMetadataName>
void swap(GroupMetadataName& lhs, GroupMetadataName& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.group_name, rhs.group_name);
}

template <typename GroupMetadataName>
bool operator!=(const GroupMetadataKey<GroupMetadataName>& lhs,
                const GroupMetadataKey<GroupMetadataName>& rhs) {
  return !operator==(lhs, rhs);
}

template <typename GroupMetadataName>
bool operator<(const GroupMetadataKey<GroupMetadataName>& lhs,
               const GroupMetadataKey<GroupMetadataName>& rhs) {
  return lhs.group_name < rhs.group_name;
}

template <typename GroupMetadataName>
bool operator>(const GroupMetadataKey<GroupMetadataName>& lhs,
               const GroupMetadataKey<GroupMetadataName>& rhs) {
  return operator<(rhs, lhs);
}

template <typename GroupMetadataName>
bool operator<=(const GroupMetadataKey<GroupMetadataName>& lhs,
                const GroupMetadataKey<GroupMetadataName>& rhs) {
  return !operator>(lhs, rhs);
}

template <typename GroupMetadataName>
bool operator>=(const GroupMetadataKey<GroupMetadataName>& lhs,
                const GroupMetadataKey<GroupMetadataName>& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GROUP_METADATA_KEY_H_
