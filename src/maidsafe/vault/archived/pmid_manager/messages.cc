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

#include "maidsafe/vault/pmid_manager/messages.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

namespace maidsafe {

namespace vault {

PmidAccountResponse::PmidAccountResponse(const std::string& serialised_pmid_accounts)
    : serialised_pmid_accounts_(serialised_pmid_accounts) {}

PmidAccountResponse::PmidAccountResponse(const PmidAccountResponse& other)
    : serialised_pmid_accounts_(other.serialised_pmid_accounts_) {}

PmidAccountResponse::PmidAccountResponse(PmidAccountResponse&& other)
    : serialised_pmid_accounts_(std::move(other.serialised_pmid_accounts_)) {}

PmidAccountResponse& PmidAccountResponse::operator=(PmidAccountResponse other) {
  swap(*this, other);
  return *this;
}

std::string PmidAccountResponse::Serialise() const { return serialised_pmid_accounts_; }

bool operator==(const PmidAccountResponse& lhs, const PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT {
  return lhs.serialised_pmid_accounts_ == rhs.serialised_pmid_accounts_;
}

void swap(PmidAccountResponse& lhs, PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.serialised_pmid_accounts_, rhs.serialised_pmid_accounts_);
}

}  // namespace vault

}  // namespace maidsafe
