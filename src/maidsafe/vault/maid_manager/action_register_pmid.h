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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_PMID_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_PMID_H_

#include <string>

#include "maidsafe/nfs/types.h"

#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/maid_manager/action_register_pmid.pb.h"
#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

class MaidManagerMetadata;

struct ActionMaidManagerRegisterPmid {
  explicit ActionMaidManagerRegisterPmid(
               const nfs_vault::PmidRegistration& pmid_registration_in,
               nfs::MessageId message_id_in);
  explicit ActionMaidManagerRegisterPmid(const std::string& serialised_action);
  ActionMaidManagerRegisterPmid(const ActionMaidManagerRegisterPmid& other);
  ActionMaidManagerRegisterPmid(ActionMaidManagerRegisterPmid&& other);
  std::string Serialise() const;

  void operator()(MaidManagerMetadata& metadata) const;

  nfs_vault::PmidRegistration pmid_registration;
  nfs::MessageId message_id;
  static const nfs::MessageAction kActionId = nfs::MessageAction::kRegisterPmidRequest;

 private:
  ActionMaidManagerRegisterPmid();
  ActionMaidManagerRegisterPmid& operator=(ActionMaidManagerRegisterPmid other);
};

bool operator==(const ActionMaidManagerRegisterPmid& lhs, const ActionMaidManagerRegisterPmid& rhs);
bool operator!=(const ActionMaidManagerRegisterPmid& lhs, const ActionMaidManagerRegisterPmid& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_PMID_H_
