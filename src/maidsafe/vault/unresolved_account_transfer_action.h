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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ACCOUNT_TRANSFER_ACTION_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ACCOUNT_TRANSFER_ACTION_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <map>
#include <set>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/nfs/types.h"

#include "maidsafe/routing/message.h"

#include "maidsafe/vault/unresolved_account_transfer_action.pb.h"

namespace maidsafe {

namespace vault {

template <typename Key, typename Action>
struct UnresolvedAccountTransferAction {
 public:
  UnresolvedAccountTransferAction(const Key& key_in, const nfs::MessageId& id_in,
                                  const std::vector<Action>& actions_in);
  explicit UnresolvedAccountTransferAction(const std::string& serialised_copy);
  //   UnresolvedAccountTransferAction(const UnresolvedAction& other);
  //   UnresolvedAccountTransferAction(UnresolvedAccountTransfer&& other);

  void Merge(const UnresolvedAccountTransferAction& other, routing::SingleId sender);

  UnresolvedAccountTransferAction GetResolvedActions(size_t resolve_num);

  std::set<routing::SingleId> GetSenders() const;

  bool IsResolved();

  std::string Serialise() const;

  Key key;
  nfs::MessageId id;
  std::vector<Action> actions;
  std::map<Action, std::pair<bool, std::set<routing::SingleId>>> entries;
  boost::posix_time::ptime time_tag;

  //   UnresolvedAccountTransferAction& operator=(UnresolvedAction other);
};

template <typename Key, typename Action>
UnresolvedAccountTransferAction<Key, Action>::UnresolvedAccountTransferAction(
    const Key& key_in, const nfs::MessageId& id_in, const std::vector<Action>& actions_in)
    : key(key_in), id(id_in), actions(actions_in), entries(),
      time_tag(boost::posix_time::microsec_clock::universal_time()) {}

template <typename Key, typename Action>
UnresolvedAccountTransferAction<Key, Action>::UnresolvedAccountTransferAction(
    const std::string& serialised_copy)
    : key(), id(), actions(), entries(),
      time_tag(boost::posix_time::microsec_clock::universal_time()) {
  protobuf::UnresolvedAccountTransferAction proto_unresolved_action;
  proto_unresolved_action.ParseFromString(serialised_copy);
  key = Key(Identity(proto_unresolved_action.serialised_key()));
  id = nfs::MessageId(proto_unresolved_action.message_id());
  for (auto& action : proto_unresolved_action.action_list())
    actions.push_back(action);
}

template <typename Key, typename Action>
void UnresolvedAccountTransferAction<Key, Action>::Merge(
    const UnresolvedAccountTransferAction& other, routing::SingleId sender) {
  auto cur_time(boost::posix_time::microsec_clock::universal_time());
  if ((cur_time - time_tag).total_seconds() > 30) {
    time_tag = cur_time;
    entries.clear();
  }
  for (auto& action : other.actions) {
    auto entry(entries.find(action));
    if (entry == std::end(entries)) {
      std::set<routing::SingleId> senders;
      entries[action] = std::make_pair(false, senders);
    }
    if (!entries[action].first)
      entries[action].second.insert(sender);
  }
}

template <typename Key, typename Action>
UnresolvedAccountTransferAction<Key, Action>
UnresolvedAccountTransferAction<Key, Action>::GetResolvedActions(size_t resolve_num) {
  std::vector<Action> resolved;
  for (auto& entry : entries) {
    LOG(kVerbose) << "before check isresolved = " << std::boolalpha << entry.second.first;
    if ((!entry.second.first) && (entry.second.second.size() >= resolve_num)) {
      entry.second.first = true;
      LOG(kVerbose) << "after assignment isresolved = " << std::boolalpha << entry.second.first;
      resolved.push_back(entry.first);
    }
  }
  return UnresolvedAccountTransferAction(key, id, resolved);
}

template <typename Key, typename Action>
std::set<routing::SingleId> UnresolvedAccountTransferAction<Key, Action>::GetSenders() const {
  std::set<routing::SingleId> senders;
  for (auto& entry : entries)
    if ((!entry.second.first) && (entry.second.second.size() > senders.size())) {
      senders.clear();
      for (auto& sender : entry.second.second)
        senders.insert(sender);
    }
  return senders;
}

template <typename Key, typename Action>
bool UnresolvedAccountTransferAction<Key, Action>::IsResolved() {
  auto cur_time(boost::posix_time::microsec_clock::universal_time());
  if ((cur_time - time_tag).total_seconds() > 30) {
    time_tag = cur_time;
    entries.clear();
    return true;
  }
  for (auto& entry : entries)
    if (!entry.second.first)
      return false;
  return true;
}

template <typename Key, typename Action>
std::string UnresolvedAccountTransferAction<Key, Action>::Serialise() const {
  protobuf::UnresolvedAccountTransferAction proto_unresolved_action;
  proto_unresolved_action.set_serialised_key(key->string());
  proto_unresolved_action.set_message_id(id.data);
  for (const auto& action : actions)
    proto_unresolved_action.add_action_list(action);
  return proto_unresolved_action.SerializeAsString();
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ACCOUNT_TRANSFER_ACTION_H_
