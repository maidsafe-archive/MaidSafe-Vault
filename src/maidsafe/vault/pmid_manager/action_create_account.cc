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


namespace maidsafe {

namespace vault {

ActionCreatePmidAccount::ActionCreatePmidAccount(const PmidName& pmid_name)
  : pmid_name_(pmid_name) {}

ActionCreatePmidAccount::ActionCreatePmidAccount(
    const std::string& serialised_action) {
  assert(serialised_action.size() == 0);
  static_cast<void>(serialised_action);
}

ActionCreatePmidAccount::ActionCreatePmidAccount(
    const ActionCreatePmidAccount& /*other*/) {}

ActionCreatePmidAccount::ActionCreatePmidAccount(
    ActionCreatePmidAccount&& /*other*/) {}

detail::DbAction ActionCreatePmidAccount::operator()(
    std::unique_ptr<PmidManagerMetadata>& metadata) {
  if (!metadata)
    metadata.reset(new PmidManagerMetadata(pmid_name_));
  return detail::DbAction::kPut;
}

std::string ActionCreatePmidAccount::Serialise() const {
  return std::string();
}

bool operator==(const ActionCreatePmidAccount& /*lhs*/,
                const ActionCreatePmidAccount& /*rhs*/) {
  return true;
}

bool operator!=(const ActionCreatePmidAccount& lhs,
                const ActionCreatePmidAccount& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
