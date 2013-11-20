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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_CREATE_REMOVE_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_CREATE_REMOVE_ACCOUNT_H_

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.pb.h"

namespace maidsafe {

namespace vault {

namespace detail {

template <bool Remove>
struct ActionCreateRemoveAccountType {
  static const nfs::MessageAction kType = nfs::MessageAction::kRemoveAccountRequest;
};

template <>
struct ActionCreateRemoveAccountType<false> {
  static const nfs::MessageAction kType = nfs::MessageAction::kCreateAccountRequest;
};

}  // namespace detail

template <bool Remove>
struct ActionCreateRemoveAccount {
  explicit ActionCreateRemoveAccount(nfs::MessageId message_id);
  explicit ActionCreateRemoveAccount(const std::string& serialised_action);
  ActionCreateRemoveAccount(const ActionCreateRemoveAccount& other);
  ActionCreateRemoveAccount(ActionCreateRemoveAccount&& other);

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = detail::ActionCreateRemoveAccountType<Remove>::kType;
  const nfs::MessageId kMessageId;
};

template <bool Remove>
bool operator==(const ActionCreateRemoveAccount<Remove>& lhs,
                const ActionCreateRemoveAccount<Remove>& rhs) {
  return (lhs.kMessageId == rhs.kMessageId);
}

template <bool Remove>
bool operator!=(const ActionCreateRemoveAccount<Remove>& lhs,
                const ActionCreateRemoveAccount<Remove>& rhs) {
  return !operator==(lhs, rhs);
}

// Implementation
template <bool Remove>
ActionCreateRemoveAccount<Remove>::ActionCreateRemoveAccount(nfs::MessageId message_id)
    : kMessageId(message_id) {}

template <bool Remove>
ActionCreateRemoveAccount<Remove>::ActionCreateRemoveAccount(const std::string& serialised_action)
    : kMessageId([&serialised_action]()->int32_t {
        protobuf::ActionMaidManagerCreateRemoveAccount action_proto;
        if (!action_proto.ParseFromString(serialised_action))
          ThrowError(CommonErrors::parsing_error);
        return action_proto.message_id();
      }()) {}

template <bool Remove>
ActionCreateRemoveAccount<Remove>::ActionCreateRemoveAccount(const ActionCreateRemoveAccount& other)
    : kMessageId(other.kMessageId) {}

template <bool Remove>
ActionCreateRemoveAccount<Remove>::ActionCreateRemoveAccount(ActionCreateRemoveAccount&& other)
    :kMessageId(std::move(other.kMessageId)) {}

template <bool Remove>
std::string ActionCreateRemoveAccount<Remove>::Serialise() const {
  protobuf::ActionMaidManagerCreateRemoveAccount action_proto;
  action_proto.set_message_id(kMessageId);
  return action_proto.SerializeAsString();
}

typedef ActionCreateRemoveAccount<false> ActionCreateAccount;
typedef ActionCreateRemoveAccount<true> ActionRemoveAccount;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_CREATE_REMOVE_ACCOUNT_H_
