/* Copyright 2012 MaidSafe.net limited

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

std::string PmidAccountResponse::Serialise() const {
  return serialised_pmid_accounts_;
}

bool operator==(const PmidAccountResponse& lhs, const PmidAccountResponse& rhs) {
  return lhs.serialised_pmid_accounts() == rhs.serialised_pmid_accounts();
}

void swap(PmidAccountResponse& lhs, PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.serialised_pmid_accounts_, rhs.serialised_pmid_accounts_);
}

}  // namespace vault

}  // namespace maidsafe
