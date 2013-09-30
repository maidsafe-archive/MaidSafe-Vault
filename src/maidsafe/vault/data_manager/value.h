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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_

#include <cstdint>
#include <set>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {

namespace vault {

// not thread safe
class DataManagerValue {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedDataManagerValueTag> serialised_type;
  explicit DataManagerValue(const serialised_type& serialised_metadata_value);
  DataManagerValue();
  serialised_type Serialise() const;

  void AddPmid(const PmidName& pmid_name);
  void RemovePmid(const PmidName& pmid_name);
  void IncrementSubscribers();
  int64_t DecrementSubscribers();
  void SetPmidOnline(const PmidName& pmid_name);
  void SetPmidOffline(const PmidName& pmid_name);
  int64_t Subscribers() const;
  std::set<PmidName> Pmids();

  friend bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

 private:
  int64_t subscribers_;
  int32_t store_failures_;
  std::set<PmidName> online_pmids_, offline_pmids_;
};

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
