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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNTS_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNTS_H_

#include <string>

#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/vault/maid_manager/action_reference_counts.pb.h"

namespace maidsafe {

namespace vault {

namespace detail {

template <bool Increment>
struct ActionMaidManagerReferenceCountsType {
  static const nfs::MessageAction kType = nfs::MessageAction::kIncrementReferenceCounts;
};

template <>
struct ActionMaidManagerReferenceCountsType<false> {
  static const nfs::MessageAction kType = nfs::MessageAction::kDecrementReferenceCounts;
};

}  // namespace detail

template <bool Increment>
struct ActionMaidManagerReferenceCounts {
  explicit ActionMaidManagerReferenceCounts(const nfs_vault::DataNames& data_names);
  explicit ActionMaidManagerReferenceCounts(const std::string& serialised_action);
  ActionMaidManagerReferenceCounts(const ActionMaidManagerReferenceCounts& other);
  ActionMaidManagerReferenceCounts(ActionMaidManagerReferenceCounts&& other);

  std::string Serialise() const;

  static const nfs::MessageAction kActionId =
      detail::ActionMaidManagerReferenceCountsType<Increment>::kType;
  const nfs_vault::DataNames kDataNames;
};

template <bool Increment>
bool operator==(const ActionMaidManagerReferenceCounts<Increment>& lhs,
                const ActionMaidManagerReferenceCounts<Increment>& rhs) {
  return (lhs.kDataNames == rhs.kDataNames);
}

template <bool Increment>
bool operator!=(const ActionMaidManagerReferenceCounts<Increment>& lhs,
                const ActionMaidManagerReferenceCounts<Increment>& rhs) {
  return !operator==(lhs, rhs);
}

// Implementation
template <bool Increment>
ActionMaidManagerReferenceCounts<Increment>::ActionMaidManagerReferenceCounts(
    const nfs_vault::DataNames& data_names)
        : kDataNames(data_names) {}

template <bool Increment>
ActionMaidManagerReferenceCounts<Increment>::ActionMaidManagerReferenceCounts(
    const std::string& serialised_action)
        : kDataNames([&serialised_action]()->nfs_vault::DataNames {
            protobuf::ActionMaidManagerReferenceCounts action_proto;
            if (!action_proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return nfs_vault::DataNames(action_proto.serialised_data_names());
          }()) {}

template <bool Increment>
ActionMaidManagerReferenceCounts<Increment>::ActionMaidManagerReferenceCounts(
    const ActionMaidManagerReferenceCounts& other)
        : kDataNames(other.kDataNames) {}

template <bool Increment>
ActionMaidManagerReferenceCounts<Increment>::ActionMaidManagerReferenceCounts(
    ActionMaidManagerReferenceCounts&& other)
        :kDataNames(std::move(other.kDataNames)) {}

template <bool Increment>
std::string ActionMaidManagerReferenceCounts<Increment>::Serialise() const {
  protobuf::ActionMaidManagerReferenceCounts action_proto;
  action_proto.set_serialised_data_names(kDataNames.Serialise());
  return action_proto.SerializeAsString();
}

typedef ActionMaidManagerReferenceCounts<true> ActionMaidManagerIncrementReferenceCounts;
typedef ActionMaidManagerReferenceCounts<false> ActionMaidManagerDecrementReferenceCounts;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNTS_H_
