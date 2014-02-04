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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UNREGISTER_PMID_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UNREGISTER_PMID_H_

#include <string>

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/maid_manager/action_unregister_pmid.pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class MaidManagerMetadata;

struct ActionMaidManagerUnregisterPmid {
  explicit ActionMaidManagerUnregisterPmid(const PmidName& pmid_name);
  explicit ActionMaidManagerUnregisterPmid(const std::string& serialised_action);
  ActionMaidManagerUnregisterPmid(const ActionMaidManagerUnregisterPmid& other);
  ActionMaidManagerUnregisterPmid(ActionMaidManagerUnregisterPmid&& other);
  std::string Serialise() const;

  void operator()(MaidManagerMetadata& metadata) const;

  friend bool operator==(const ActionMaidManagerUnregisterPmid& lhs,
                         const ActionMaidManagerUnregisterPmid& rhs);

  static const nfs::MessageAction kActionId = nfs::MessageAction::kUnregisterPmidRequest;

 private:
  ActionMaidManagerUnregisterPmid();
  ActionMaidManagerUnregisterPmid& operator=(ActionMaidManagerUnregisterPmid other);

  const PmidName kPmidName;
};

bool operator==(const ActionMaidManagerUnregisterPmid& lhs,
                const ActionMaidManagerUnregisterPmid& rhs);

bool operator!=(const ActionMaidManagerUnregisterPmid& lhs,
                const ActionMaidManagerUnregisterPmid& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_UNREGISTER_PMID_H_
