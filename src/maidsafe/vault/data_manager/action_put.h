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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_ACTION_PUT_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_ACTION_PUT_H_

#include <cstdint>
#include <string>

#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/data_manager/data_manager.h"

namespace maidsafe {

namespace vault {

class DataManagerValue;

struct ActionDataManagerPut {
  ActionDataManagerPut(uint64_t size, nfs::MessageId message_id);
  explicit ActionDataManagerPut(const std::string& serialised_action);
  ActionDataManagerPut(const ActionDataManagerPut& other);
  ActionDataManagerPut(ActionDataManagerPut&& other);
  std::string Serialise() const;

  detail::DbAction operator()(std::unique_ptr<DataManagerValue>& value);

  static const nfs::MessageAction kActionId = nfs::MessageAction::kPutRequest;
  const uint64_t kSize;
  const nfs::MessageId kMessageId;

 private:
  ActionDataManagerPut();
  ActionDataManagerPut& operator=(ActionDataManagerPut other);
};

bool operator==(const ActionDataManagerPut& lhs, const ActionDataManagerPut& rhs);
bool operator!=(const ActionDataManagerPut& lhs, const ActionDataManagerPut& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_ACTION_PUT_H_
