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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/error.h"

#include "maidsafe/vault/unresolved_action.pb.h"

namespace maidsafe {

namespace vault {

template <typename Key, typename Action>
struct UnresolvedAction {
  UnresolvedAction(const std::string& serialised_copy, const NodeId& sender_id,
                   const NodeId& this_node_id);
  UnresolvedAction(const UnresolvedAction& other);
  UnresolvedAction(UnresolvedAction&& other);
  UnresolvedAction(const Key& key_in, const Action& action_in, const NodeId& this_node_id,
                   int32_t this_entry_id);
  std::string Serialise() const;
  bool IsReadyForSync() const;

  Key key;
  Action action;
  std::pair<NodeId, int32_t> this_node_and_entry_id;
  std::vector<std::pair<NodeId, int32_t>> peer_and_entry_ids;
  bool sent_to_peers;
  int sync_counter;

 private:
  UnresolvedAction& operator=(UnresolvedAction other);

  // Helpers to handle Action class with/without Serialise() member function.
  template <typename T, typename Signature>
  class HasSerialise {
   private:
    typedef char Yes[1];
    typedef char No[2];
    template <typename U, U>
    struct type_check;
    template <typename V>
    static Yes& Check(type_check<Signature, &V::Serialise>*);
    template <typename>
    static No& Check(...);

   public:
    static bool const value = sizeof(Check<T>(0)) == sizeof(Yes);
  };

  template <typename T>
  typename std::enable_if<HasSerialise<T, std::string (T::*)() const>::value, void>::type
  SerialiseAction(protobuf::UnresolvedAction& proto_unresolved_action) const {
    proto_unresolved_action.set_serialised_action(action.Serialise());
  }

  template <typename T>
  typename std::enable_if<!HasSerialise<T, std::string (T::*)() const>::value, void>::type
  SerialiseAction(protobuf::UnresolvedAction& /*proto_unresolved_action*/) const {}

  template <typename T>
  typename std::enable_if<HasSerialise<T, std::string (T::*)() const>::value, T>::type ParseAction(
      const std::string& serialised_copy) const {
    protobuf::UnresolvedAction proto_unresolved_action;
    if (!proto_unresolved_action.ParseFromString(serialised_copy))
      ThrowError(CommonErrors::parsing_error);
    return T(proto_unresolved_action.serialised_action());
  }

  template <typename T>
  typename std::enable_if<!HasSerialise<T, std::string (T::*)() const>::value, T>::type ParseAction(
      const std::string& /*serialised_copy*/) const {}
};

// ==================== Implementation =============================================================
template <typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const std::string& serialised_copy,
                                                const NodeId& sender_id, const NodeId& this_node_id)
    : key(),
      action(ParseAction<Action>(serialised_copy)),
      this_node_and_entry_id(),
      peer_and_entry_ids(),
      sent_to_peers(false),
      sync_counter(0) {
  protobuf::UnresolvedAction proto_unresolved_action;
  proto_unresolved_action.ParseFromString(serialised_copy);
  key = Key(proto_unresolved_action.serialised_key());
  if (sender_id == this_node_id)
    this_node_and_entry_id = std::make_pair(this_node_id, proto_unresolved_action.entry_id());
  else
    peer_and_entry_ids.push_back(std::make_pair(sender_id, proto_unresolved_action.entry_id()));
}

template <typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const UnresolvedAction& other)
    : key(other.key),
      action(other.action),
      this_node_and_entry_id(other.this_node_and_entry_id),
      peer_and_entry_ids(other.peer_and_entry_ids),
      sent_to_peers(other.sent_to_peers),
      sync_counter(other.sync_counter) {}

template <typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(UnresolvedAction&& other)
    : key(std::move(other.key)),
      action(std::move(other.action)),
      this_node_and_entry_id(std::move(other.this_node_and_entry_id)),
      peer_and_entry_ids(std::move(other.peer_and_entry_ids)),
      sent_to_peers(std::move(other.sent_to_peers)),
      sync_counter(std::move(other.sync_counter)) {}

template <typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const Key& key_in, const Action& action_in,
                                                const NodeId& this_node_id, int32_t this_entry_id)
    : key(key_in),
      action(action_in),
      this_node_and_entry_id(std::make_pair(this_node_id, this_entry_id)),
      peer_and_entry_ids(),
      sent_to_peers(false),
      sync_counter(0) {}

template <typename Key, typename Action>
std::string UnresolvedAction<Key, Action>::Serialise() const {
  protobuf::UnresolvedAction proto_unresolved_action;
  proto_unresolved_action.set_serialised_key(key.Serialise());
  SerialiseAction<Action>(proto_unresolved_action);
  proto_unresolved_action.set_entry_id(this_node_and_entry_id.second);
  return proto_unresolved_action.SerializeAsString();
}

template <typename Key, typename Action>
bool UnresolvedAction<Key, Action>::IsReadyForSync() const {
  // TODO(Fraser#5#): 2013-07-22 - Confirm sync_counter limit and remove magic number
  return !this_node_and_entry_id.first.IsZero() && sync_counter < 10;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ACTION_H_
