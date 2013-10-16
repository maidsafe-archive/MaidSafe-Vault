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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_HELPERS_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_HELPERS_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "maidsafe/common/config.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/vault/pmid_registration.h"

#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {

namespace vault {

struct PmidRegistrationOp {
  PmidRegistrationOp(std::unique_ptr<MaidManager::UnresolvedRegisterPmid>&& /*synced_action_in*/)
      : mutex(),
//        synced_action(std::move(synced_action_in)),
//        pmid_registration(pmid_registration_in),
        public_maid(),
        public_pmid(),
        count(0) {}
  template <typename PublicFobType>
  void SetPublicFob(std::unique_ptr<PublicFobType>&&);
  std::mutex mutex;
//  std::unique_ptr<MaidManager::UnresolvedRegisterPmid> synced_action;
//  nfs_vault::PmidRegistration pmid_registration;
  std::unique_ptr<passport::PublicMaid> public_maid;
  std::unique_ptr<passport::PublicPmid> public_pmid;
  int count;
};

template <>
void PmidRegistrationOp::SetPublicFob<passport::PublicMaid>(
    std::unique_ptr<passport::PublicMaid>&& pub_maid);

template <>
void PmidRegistrationOp::SetPublicFob<passport::PublicPmid>(
    std::unique_ptr<passport::PublicPmid>&& pub_pmid);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_HELPERS_H_
