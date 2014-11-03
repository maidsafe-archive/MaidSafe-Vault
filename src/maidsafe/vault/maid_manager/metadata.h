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
#include "maidsafe/vault/maid_manager/pmid_totals.h"

namespace maidsafe {

namespace vault {

namespace test {
class MaidManagerServiceTest;
}

struct PmidManagerMetadata;

class MaidManagerMetadata {
 public:
  enum class Status {
    kOk,
    kLowSpace,
    kNoSpace
  };
  MaidManagerMetadata();
  MaidManagerMetadata(int64_t data_stored, int64_t space_available);
  MaidManagerMetadata(const MaidManagerMetadata& other);
  MaidManagerMetadata(MaidManagerMetadata&& other);
  MaidManagerMetadata& operator=(MaidManagerMetadata other);
  explicit MaidManagerMetadata(const std::string& serialised_metadata_value);

  std::string Serialise() const;
  template <typename Data>
  Status AllowPut(const Data& data) const;
  void PutData(int64_t size);
  void DeleteData(int64_t size);
  void RegisterPmid(const nfs_vault::PmidRegistration& pmid_registration);
  void UnregisterPmid(const PmidName& pmid_name);
  void UpdatePmidTotals(const PmidManagerMetadata& pmid_metadata);
  std::string Print() const;

  friend void swap(MaidManagerMetadata& lhs, MaidManagerMetadata& rhs);
  friend bool operator==(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs);
  friend class test::MaidManagerServiceTest;

 private:
  int64_t data_stored_;
  int64_t space_available_;
};


template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicPmid& data) const;
template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicMaid& data) const;
template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const passport::PublicAnmaid& data) const;


template <typename Data>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(const Data& data) const {
  auto size(data.Serialise()->string().size());
  LOG(kVerbose) << "MaidManagerMetadata::AllowPut data " << HexSubstr(data.name().value)
                << " has size of " << size << " trying to put into account provding "
                << space_available_ << " total available_size by far";
  if (space_available_ < (static_cast<int64_t>(data_stored_ + size)))
    return Status::kNoSpace;

  return ((3 * space_available_ / 100) < static_cast<int64_t>(data_stored_ + size))
             ? Status::kLowSpace : Status::kOk;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_METADATA_H_
