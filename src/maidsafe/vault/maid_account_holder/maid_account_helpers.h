/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HELPERS_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HELPERS_H_

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/pmid_account_holder/pmid_record.h"


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
      : kMaidAccountName(maid_account_name),
        kPmidAccountName(pmid_account_name),
        mutex(),
        pmid_records() {}
  const MaidName kMaidAccountName;
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

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HELPERS_H_
