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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UPDATE_PMID_HEALTH_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UPDATE_PMID_HEALTH_H_

#include <cstdint>
#include <string>

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/maid_manager/metadata.h"

namespace maidsafe {

namespace vault {

struct ActionMaidManagerUpdatePmidHealth {
  explicit ActionMaidManagerUpdatePmidHealth(const PmidName& pmid_name,
                                             const PmidManagerMetadata& pmid_health_in);
  explicit ActionMaidManagerUpdatePmidHealth(const std::string& serialised_action);
  ActionMaidManagerUpdatePmidHealth(const ActionMaidManagerUpdatePmidHealth& other);
  ActionMaidManagerUpdatePmidHealth(ActionMaidManagerUpdatePmidHealth&& other);

  std::string Serialise() const;
  void operator()(MaidManagerMetadata& metadata);
  static const nfs::MessageAction kActionId = nfs::MessageAction::kPmidHealthResponse;
  const PmidName kPmidName;
  const PmidManagerMetadata kPmidHealth;
};

bool operator==(const ActionMaidManagerUpdatePmidHealth& lhs,
                const ActionMaidManagerUpdatePmidHealth& rhs);

bool operator!=(const ActionMaidManagerUpdatePmidHealth& lhs,
                const ActionMaidManagerUpdatePmidHealth& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UPDATE_PMID_HEALTH_H_
