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

#ifndef MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"

namespace maidsafe {

namespace vault {


template <typename UnresolvedAccountTransfer, typename UnresolvedStaticMetadata>
class AccountTransfer {
 public:
  AccountTransfer(const NodeId& node_id);
  // For Data manager like personas
  std::vector<UnresolvedAccountTransfer> AddUnresolved(
          std::vector<UnresolvedAccountTransfer>&& unresolved_actions);
  // For MaidManager like personas
  std::pair<UnresolvedStaticMetadata, std::vector<UnresolvedAccountTransfer>> AddUnresolved(
      const UnresolvedStaticMetadata& metadata,
      std::vector<UnresolvedAccountTransfer>&& unresolved_actions);
  // To handle double churn situations
  void ReplaceNode(const NodeId& old_node, const NodeId& new_node);

 private:
  AccountTransfer(AccountTransfer&&);
  AccountTransfer(const AccountTransfer&);
  AccountTransfer& operator=(AccountTransfer other);

  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<UnresolvedAccountTransfer>> unresolved_actions_;
  std::vector<std::unique_ptr<UnresolvedMetadata>> unresolved_metdata_;
  const NodeId kNodeId_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
