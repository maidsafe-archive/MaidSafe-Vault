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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_

#include <cstdint>
#include <set>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

// not thread safe
class DataManagerValue {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedDataManagerValueTag> serialised_type;
  explicit DataManagerValue(const serialised_type& serialised_metadata_value);
  DataManagerValue(const PmidName& pmid_name, int size_in);
  serialised_type Serialise() const;

  void AddPmid(const PmidName& pmid_name);
  void RemovePmid(const PmidName& pmid_name);
  void IncrementSubscribers();
  int64_t DecrementSubscribers();
  void SetPmidOnline(const PmidName& pmid_name);
  void SetPmidOffline(const PmidName& pmid_name);

  friend bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

 private:
  int data_size_;
  int64_t subscribers_;
  std::set<PmidName> online_pmids_, offline_pmids_;
};

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
