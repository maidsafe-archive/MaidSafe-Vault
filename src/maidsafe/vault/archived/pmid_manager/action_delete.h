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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_ACTION_DELETE_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_ACTION_DELETE_H_

#include <cstdint>
#include <string>

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {
namespace vault {

struct ActionPmidManagerDelete {
  ActionPmidManagerDelete(uint64_t size, bool pmid_node_available_in, bool data_failure);
  explicit ActionPmidManagerDelete(const std::string& serialised_action);
  void operator()(PmidManagerValue& value);
  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kDeleteRequest;
  uint64_t kSize;
  bool pmid_node_available;
  bool data_failure;
};

bool operator==(const ActionPmidManagerDelete& lhs, const ActionPmidManagerDelete& rhs);
bool operator!=(const ActionPmidManagerDelete& lhs, const ActionPmidManagerDelete& rhs);

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_ACTION_DELETE_H_
