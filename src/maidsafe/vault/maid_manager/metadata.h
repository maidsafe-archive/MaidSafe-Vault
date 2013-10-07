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
  enum class Status {
    kOk,
    kLowSpace,
    kNoSpace
  };
  MaidManagerMetadata();
  MaidManagerMetadata(int64_t total_put_data, const std::vector<PmidTotals>& pmid_totals);
  MaidManagerMetadata(const MaidManagerMetadata& other);
  MaidManagerMetadata(MaidManagerMetadata&& other);
  MaidManagerMetadata& operator=(MaidManagerMetadata other);
  explicit MaidManagerMetadata(const std::string& serialised_metadata_value);

  std::string Serialise() const;
  template <typename Data>
  Status AllowPut(const Data& data) const;
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


template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicPmid& data) const;
template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicMaid& data) const;
template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicAnmaid& data) const;


template <typename Data>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const Data& data) const {
  int64_t total_claimed_available_size_by_pmids(0);
  for (const auto& pmid_total : pmid_totals_)
    total_claimed_available_size_by_pmids += pmid_total.pmid_metadata.claimed_available_size;
  auto cost(data.data().string().size());
  if (total_claimed_available_size_by_pmids < total_put_data_ + cost)
    return Status::kNoSpace;

  return ((total_claimed_available_size_by_pmids / 100) * 3 < total_put_data_ + cost)
             ? Status::kLowSpace
             : Status::kOk;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_
