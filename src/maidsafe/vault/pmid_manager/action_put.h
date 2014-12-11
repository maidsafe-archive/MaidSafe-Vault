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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_ACTION_PUT_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_ACTION_PUT_H_

#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/config.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

struct ActionPmidManagerPut {
  ActionPmidManagerPut(int32_t size, nfs::MessageId message_id);
  explicit ActionPmidManagerPut(const std::string& serialised_action);
  ActionPmidManagerPut(const ActionPmidManagerPut& other);
  ActionPmidManagerPut(ActionPmidManagerPut&& other);

  void operator()(PmidManagerValue& value);

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kPutRequest;
  const int32_t kSize;
  const nfs::MessageId kMessageId;

 private:
  ActionPmidManagerPut();
  ActionPmidManagerPut& operator=(ActionPmidManagerPut other);
};

bool operator==(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs);
bool operator!=(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_ACTION_PUT_H_
