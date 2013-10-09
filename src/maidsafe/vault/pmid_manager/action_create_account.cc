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

#include "maidsafe/vault/pmid_manager/action_create_account.h"
#include "maidsafe/vault/pmid_manager/action_create_account.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {
namespace vault {

ActionPmidManagerCreateAccount::ActionPmidManagerCreateAccount() {}

ActionPmidManagerCreateAccount::ActionPmidManagerCreateAccount(
    const std::string& /*serialised_action*/) {}

ActionPmidManagerCreateAccount::ActionPmidManagerCreateAccount(
    const ActionPmidManagerCreateAccount& /*other*/) {}

ActionPmidManagerCreateAccount::ActionPmidManagerCreateAccount(
  ActionPmidManagerCreateAccount&& /*other*/) {}

std::string ActionPmidManagerCreateAccount::Serialise() const {
  protobuf::ActionPmidManagerCreateAccount action_put_proto;
  return action_put_proto.SerializeAsString();
}

detail::DbAction ActionPmidManagerCreateAccount::operator()(
    boost::optional<PmidManagerMetadata>& metadata) {
  if (metadata)
    ThrowError(VaultErrors::account_already_exist);
  metadata.reset(PmidManagerMetadata());
  return detail::DbAction::kPut;
}

bool operator==(const ActionPmidManagerCreateAccount& /*lhs*/,
                const ActionPmidManagerCreateAccount& /*rhs*/) {
  return true;
}

bool operator!=(const ActionPmidManagerCreateAccount& lhs,
                const ActionPmidManagerCreateAccount& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
