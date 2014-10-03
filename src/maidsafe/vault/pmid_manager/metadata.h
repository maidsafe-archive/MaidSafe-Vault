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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_METADATA_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_METADATA_H_

#include <cstdint>
#include <string>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/client/messages.h"
#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

struct PmidManagerMetadata {
 public:
  PmidManagerMetadata();
  explicit PmidManagerMetadata(const PmidName& pmid_name_in);
  explicit PmidManagerMetadata(const std::string& serialised_metadata);
  explicit PmidManagerMetadata(maidsafe_error error);
  PmidManagerMetadata(const PmidManagerMetadata& other);
  PmidManagerMetadata(PmidManagerMetadata&& other);
  PmidManagerMetadata& operator=(PmidManagerMetadata other);
  void PutData(int32_t size);
  void DeleteData(int32_t size);
  void HandleLostData(int32_t size);
  void HandleFailure(int32_t size);
  void SetAvailableSize(const int64_t& available_size);
  std::string Serialise() const;
  detail::GroupDbMetaDataStatus GroupStatus();
  std::string Print() const;

  PmidName pmid_name;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
  int64_t claimed_available_size;
  nfs_client::ReturnCode return_code;
};

bool operator==(const PmidManagerMetadata& lhs, const PmidManagerMetadata& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_METADATA_H_
