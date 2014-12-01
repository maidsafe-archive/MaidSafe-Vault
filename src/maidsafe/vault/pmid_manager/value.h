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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_VALUE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/config.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

struct PmidManagerValue {
 public:
  PmidManagerValue();
  PmidManagerValue(const uint64_t& stored_total_size_in, const uint64_t& lost_total_size_in,
                   const uint64_t& offered_space_in);
  explicit PmidManagerValue(const std::string& serialised_value);
  PmidManagerValue(const PmidManagerValue& other);
  PmidManagerValue(PmidManagerValue&& other);
  PmidManagerValue& operator=(PmidManagerValue other);
  void PutData(uint64_t size);
  void DeleteData(uint64_t size);
  void HandleLostData(uint64_t size);
  void HandleFailure(uint64_t size);
  void SetAvailableSize(const int64_t& available_size);
  void UpdateAccount(int32_t diff_size);
  std::string Serialise() const;
  detail::GroupDbMetaDataStatus GroupStatus();
  std::string Print() const;
  static PmidManagerValue Resolve(const std::vector<PmidManagerValue>& values);

  uint64_t stored_total_size;
  uint64_t lost_total_size;
  uint64_t offered_space;
};

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_VALUE_H_
