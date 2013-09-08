/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/helpers.h"


namespace maidsafe {

namespace vault {

struct PmidManagerMetadata;

class MaidManagerMetadata {
 public:
  enum class Status { kOk, kLowSpace, kNoSpace };
  MaidManagerMetadata();
  MaidManagerMetadata(int64_t total_put_data, const std::vector<PmidTotals>& pmid_totals);
  MaidManagerMetadata(const MaidManagerMetadata& other);
  MaidManagerMetadata(MaidManagerMetadata&& other);
  MaidManagerMetadata& operator=(MaidManagerMetadata other);
  explicit MaidManagerMetadata(const std::string& serialised_metadata_value);

  std::string Serialise() const;

  Status AllowPut(int32_t cost) const;
  void PutData(int32_t cost);
  void DeleteData(int32_t cost);
  void RegisterPmid(const nfs_vault::PmidRegistration& pmid_registration);
  void UnregisterPmid(const nfs_vault::PmidRegistration& pmid_registration);
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
