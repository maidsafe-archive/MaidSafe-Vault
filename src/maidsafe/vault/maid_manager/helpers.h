/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_HELPERS_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_HELPERS_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/pmid_manager/pmid_record.h"


namespace maidsafe {

namespace vault {

struct PmidRegistrationOp {
  PmidRegistrationOp(const nfs::PmidRegistration& pmid_registration_in,
                     const routing::ReplyFunctor& reply_functor_in)
      : mutex(),
        pmid_registration(pmid_registration_in),
        reply_functor(reply_functor_in),
        public_maid(),
        public_pmid(),
        count(0) {}
  template<typename PublicFobType>
  void SetPublicFob(std::unique_ptr<PublicFobType>&&);
  std::mutex mutex;
  nfs::PmidRegistration pmid_registration;
  routing::ReplyFunctor reply_functor;
  std::unique_ptr<passport::PublicMaid> public_maid;
  std::unique_ptr<passport::PublicPmid> public_pmid;
  int count;
};

template<>
void PmidRegistrationOp::SetPublicFob<passport::PublicMaid>(
    std::unique_ptr<passport::PublicMaid>&& pub_maid);

template<>
void PmidRegistrationOp::SetPublicFob<passport::PublicPmid>(
    std::unique_ptr<passport::PublicPmid>&& pub_pmid);

struct GetPmidTotalsOp {
  GetPmidTotalsOp(const MaidName& maid_account_name, const PmidName& pmid_account_name)
      : kMaidManagerName(maid_account_name),
        kPmidAccountName(pmid_account_name),
        mutex(),
        pmid_records() {}
  const MaidName kMaidManagerName;
  const PmidName kPmidAccountName;
  std::mutex mutex;
  std::vector<PmidRecord> pmid_records;
};

struct PmidTotals {
  PmidTotals();
  explicit PmidTotals(
      const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in);
  PmidTotals(const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in,
             const PmidRecord& pmid_record_in);
  PmidTotals(const PmidTotals& other);
  PmidTotals(PmidTotals&& other);
  PmidTotals& operator=(PmidTotals other);

  nfs::PmidRegistration::serialised_type serialised_pmid_registration;
  PmidRecord pmid_record;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_HELPERS_H_
