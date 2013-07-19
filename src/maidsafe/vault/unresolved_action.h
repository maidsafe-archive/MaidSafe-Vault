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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_

#include <cstdint>
#include <set>
#include <string>
#include <utility>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/error.h"

#include "maidsafe/vault/unresolved_action.pb.h"


namespace maidsafe {

namespace vault {

template<typename Key, typename Action>
struct UnresolvedAction {
  UnresolvedAction(const std::string& serialised_copy,
                   const NodeId& sender_id,
                   const NodeId& this_node_id);
  UnresolvedAction(const UnresolvedAction& other);
  UnresolvedAction(UnresolvedAction&& other);
  UnresolvedAction(const Key& key_in,
                   const Action& action_in,
                   const NodeId& this_node_id,
                   int32_t this_entry_id);
  std::string Serialise() const;
  bool HasThisNode() const;

  Key key;
  Action action;
  std::pair<NodeId, int32_t> this_node_and_entry_id;
  std::set<std::pair<NodeId, int32_t>> peer_and_entry_ids;
  int sync_counter;

 private:
  UnresolvedAction& operator=(UnresolvedAction other);
};



template<typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const std::string& serialised_copy,
                                                const NodeId& sender_id,
                                                const NodeId& this_node_id)
    : key(),
      action([&serialised_copy]()->Action {
        protobuf::UnresolvedAction proto_unresolved_action;
        if (!proto_unresolved_action.ParseFromString(serialised_copy))
          ThrowError(CommonErrors::parsing_error);
        return Action(proto_unresolved_action.serialised_action());
      }()),
      this_node_and_entry_id(),
      peer_and_entry_ids(),
      sync_counter(0) {
  protobuf::UnresolvedAction proto_unresolved_action;
  proto_unresolved_action.ParseFromString(serialised_copy);
  key = Key(proto_unresolved_action.serialised_key());
  if (sender_id == this_node_id)
    this_node_and_entry_id = std::make_pair(this_node_id, proto_unresolved_action.entry_id());
  else
    peer_and_entry_ids.insert(std::make_pair(sender_id, proto_unresolved_action.entry_id()));
}

template<typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const UnresolvedAction& other)
    : key(other.key),
      action(other.action),
      this_node_and_entry_id(other.this_node_and_entry_id),
      peer_and_entry_ids(other.peer_and_entry_ids),
      sync_counter(other.sync_counter) {}

template<typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(UnresolvedAction&& other)
    : key(std::move(other.key)),
      action(std::move(other.action)),
      this_node_and_entry_id(std::move(other.this_node_and_entry_id)),
      peer_and_entry_ids(std::move(other.peer_and_entry_ids)),
      sync_counter(std::move(other.sync_counter)) {}

template<typename Key, typename Action>
UnresolvedAction<Key, Action>::UnresolvedAction(const Key& key_in,
                                                const Action& action_in,
                                                const NodeId& this_node_id,
                                                int32_t this_entry_id)
    : key(key_in),
      action(action_in),
      this_node_and_entry_id(std::make_pair(this_node_id, this_entry_id)),
      peer_and_entry_ids(),
      sync_counter(0) {}

template<typename Key, typename Action>
std::string UnresolvedAction<Key, Action>::Serialise() const {
  protobuf::UnresolvedAction proto_unresolved_action;
  proto_unresolved_action.set_serialised_key(key.Serialise());
  proto_unresolved_action.set_serialised_action(action.Serialise());
  proto_unresolved_action.set_entry_id(this_node_and_entry_id.second);
  return proto_unresolved_action.SerializeAsString();
}

template<typename Key, typename Action>
bool UnresolvedAction<Key, Action>::HasThisNode() const {
  return !this_node_and_entry_id.first.IsZero();
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
