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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_MESSAGES_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_MESSAGES_H_

#include <cstdint>
#include <string>

#include "maidsafe/common/error.h"

namespace maidsafe {
namespace vault {

class PmidAccountResponse {
 public:
  explicit PmidAccountResponse(const std::string& serialised_pmid_accounts);
  PmidAccountResponse(const PmidAccountResponse& other);
  PmidAccountResponse(PmidAccountResponse&& other);
  PmidAccountResponse& operator=(PmidAccountResponse other);

  std::string Serialise() const;

  friend void swap(PmidAccountResponse& lhs, PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT;
  friend bool operator==(const PmidAccountResponse& lhs,
                         const PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT;

 private:
  PmidAccountResponse();
  std::string serialised_pmid_accounts_;
};

bool operator==(const PmidAccountResponse& lhs, const PmidAccountResponse& rhs) MAIDSAFE_NOEXCEPT;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_MESSAGES_H_
