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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_ACTION_REMOVE_PMID_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_ACTION_REMOVE_PMID_H_

#include <cstdint>
#include <string>

#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class MaidManagerValue;
class DataManagerValue;

struct ActionDataManagerRemovePmid {
  explicit ActionDataManagerRemovePmid(const PmidName& pmid_name);
  explicit ActionDataManagerRemovePmid(const std::string& serialised_action);
  ActionDataManagerRemovePmid(const ActionDataManagerRemovePmid& other);
  ActionDataManagerRemovePmid(ActionDataManagerRemovePmid&& other);
  std::string Serialise() const;

  detail::DbAction operator()(std::unique_ptr<DataManagerValue>& value);

  static const nfs::MessageAction kActionId = nfs::MessageAction::kRemovePmid;
  const PmidName kPmidName;

 private:
  ActionDataManagerRemovePmid();
  ActionDataManagerRemovePmid& operator=(ActionDataManagerRemovePmid other);
};

bool operator==(const ActionDataManagerRemovePmid& lhs, const ActionDataManagerRemovePmid& rhs);

bool operator!=(const ActionDataManagerRemovePmid& lhs, const ActionDataManagerRemovePmid& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_ACTION_REMOVE_PMID_H_
