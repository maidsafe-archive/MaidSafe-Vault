/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_VALUE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/nfs/vault/messages.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {


namespace test {
  class MpidManagerServiceTest;
}

class MpidManagerValue {
 public:
  MpidManagerValue();
  MpidManagerValue(const MpidManagerValue& other);
  MpidManagerValue(MpidManagerValue&& other);
  MpidManagerValue& operator=(MpidManagerValue other);
  explicit MpidManagerValue(const std::string& serialised_value);

  void AddAlert(const nfs_vault::MpidMessageAlert& alert);
  void RemoveAlert(const nfs_vault::MpidMessageAlert& alert);
  void AddMessage(const nfs_vault::MpidMessage& alert);
  void RemoveMessage(const nfs_vault::MpidMessageAlert& alert);

  std::string Serialise() const;

  static MpidManagerValue Resolve(const std::vector<MpidManagerValue>& values);

  friend void swap(MpidManagerValue& lhs, MpidManagerValue& rhs);
  friend bool operator==(const MpidManagerValue& lhs, const MpidManagerValue& rhs);

 private:
  std::vector<nfs_vault::MpidMessage> outbox_;
  std::vector<nfs_vault::MpidMessageAlert> inbox_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_VALUE_H_
