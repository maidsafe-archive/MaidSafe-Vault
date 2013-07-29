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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/helpers.h"


namespace maidsafe {

namespace vault {

struct PmidManagerMetadata;

class MaidManagerMetadata {
 public:
  MaidManagerMetadata();
  MaidManagerMetadata(int64_t total_put_data, const std::vector<PmidTotals>& pmid_totals);
  MaidManagerMetadata(const MaidManagerMetadata& other);
  MaidManagerMetadata(MaidManagerMetadata&& other);
  MaidManagerMetadata& operator=(MaidManagerMetadata other);
  explicit MaidManagerMetadata(const std::string& serialised_metadata_value);

  std::string Serialise() const;

  void PutData(int32_t cost);
  void DeleteData(int32_t cost);
  void RegisterPmid(const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const nfs::PmidRegistration& pmid_registration);
  void UpdatePmidTotals(const PmidManagerMetadata& pmid_metadata);

  friend void swap(MaidManagerMetadata& lhs, MaidManagerMetadata& rhs);
  friend bool operator==(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs);

 private:
  std::vector<PmidTotals>::iterator Find(const PmidName& pmid_name);

  int64_t total_put_data_;
  std::vector<PmidTotals> pmid_totals_;
};

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_
