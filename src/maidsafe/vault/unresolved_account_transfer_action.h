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

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/unresolved_action.pb.h"

namespace maidsafe {

namespace vault {



template <typename Key, typename Action>
struct UnresolvedAccountTransferAction {
  typedef Action ActionType;
  typedef Key KeyType;

  // To create unresolved action from received key value and metadata combination from peer
  UnresolvedAccountTransferAction(const key& key_in, const Action& action_in,
                                  const NodeId& sender_id);

  UnresolvedAccountTransferAction(const UnresolvedAction& other);
  UnresolvedAccountTransferAction(UnresolvedAccountTransfer&& other);

//  std::string Serialise() const; // not required as it is created on destination node

  Key key;
  Action action;
  std::vector<NodeId> peers_;

 private:
  UnresolvedAccountTransferAction& operator=(UnresolvedAction other);
};


// TODO extend to unresolved static metadata action and unresolved db action


// Discuss If its good to send unresolved action from the source or a bulk of
// key/value/metadata in a specilaised container.
// Sending in form of repeated unresolved action is a good for generalisation but wasteful, as
// in case of maid manager type persona half of the db key string will be same for a given account.

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ACCOUNT_TRANSFER_ACTION_H_
